#ifndef MT_Maze_Student_Solver_H
#define MT_Maze_Student_Solver_H

#include <list>
#include <vector>
#include <thread>
#include <chrono>


#include "Maze.h"
#include "MazeSolver.h"
#include "Direction.h"

const int threadNum = 168;

// Feel free to change your class any way you see fit
// It needs to inherit at some point from the MazeSolver
class MTMazeStudentSolver : public MazeSolver 
{
public:
	MTMazeStudentSolver(Maze *maze) 
	: MazeSolver( maze )
	{
		assert(pMaze);
	}

	MTMazeStudentSolver() = delete;
	MTMazeStudentSolver(const MTMazeStudentSolver &) = delete;
	MTMazeStudentSolver &operator=(const MTMazeStudentSolver &) = delete;
	~MTMazeStudentSolver() = default;

	void setDirectionRouteBT(Position at, Direction from)
	{
		unsigned int val = pMaze->getCell(at);
		unsigned int dir = 0;

		switch (from)
		{
		case Direction::East:
			dir = 0x01000000;
			break;
		case Direction::West:
			dir = 0x02000000;
			break;
		case Direction::South:
			dir = 0x04000000;
			break;
		case Direction::North:
			dir = 0x08000000;
			break;
		case Direction::Uninitialized:
			break;
		default:
			assert(0);
			break;
		}

		val |= dir;
		pMaze->setCell(at, val);
	}

	Direction getDirectionRouteBT(Position at)
	{
		unsigned int val = pMaze->getCell(at);
		val &= 0x0F000000;  // mask BT bits

		switch (val)
		{
		case 0x01000000:
			return Direction::East;
		case 0x02000000:
			return Direction::West;
		case 0x04000000:
			return Direction::South;
		case 0x08000000:
			return Direction::North;
		case 0x00000000:
			return Direction::Uninitialized;
		default:
			assert(0);
			return Direction::Uninitialized;
		}
	}

	Junction firstJunction_TB(Position pos, int threadId)
	{
		Branches branches = pMaze->getBranches(pos, threadId);

		int size = branches.size();

		if (size == 1)
		{
			Direction tmp = branches.getNext();
			return followPath_TB(pos, tmp, threadId);
		}
		else // size != 0
		{
			return Junction(pos, Direction::Uninitialized, branches);
		}
	}

	Junction followPath_TB(Position at, Direction dir, int threadId)
	{
		Branches branches;
		Direction go_to = dir;
		Direction came_from = reverseDir(dir);
		at = at.move(go_to);

		while (true)
		{
			// 1. Check if we hit the target
			if (at == pMaze->getEnd())
			{
				// Return a junction with empty branches
				return Junction(at, came_from, branches);
			}

			// 2. Get moves from current spot
			branches = pMaze->getBranches(at, threadId);
			branches.remove(came_from);

			// 3. It's a junction or dead end
			int size = branches.size();
			if (size > 1)
			{
				Direction mark = this->getDirectionRouteBT(at);
				bool isOverlap = (mark != Direction::Uninitialized) && (mark != go_to);
				return Junction(at, came_from, branches, isOverlap);
			}
			else if (size == 0)
			{
				return Junction(at, came_from, branches);
			}

			// 4. It is a corridor, keep walking
			go_to = branches.getNext();
			at = at.move(go_to);
			came_from = reverseDir(go_to);
		}
	}

	Junction firstJunction_BT(Position pos, int threadId)
	{
		Branches branches = pMaze->getBranches(pos, threadId);

		int size = branches.size();

		if (size == 1)
		{
			Direction tmp = branches.getNext();
			return followPath_BT(pos, tmp, threadId);
		}
		else // size != 0
		{
			return Junction(pos, Direction::Uninitialized, branches);
		}
	}

	Junction followPath_BT(Position at, Direction dir, int threadId)
	{
		Branches branches;
		Direction go_to = dir;
		Direction came_from = reverseDir(dir);

		at = at.move(go_to);

		while (true)
		{
			// Note: BT usually doesn't check for "Start" here unless you want early exit.
			// BT's job is mostly to mark the path.
			branches = pMaze->getBranches(at, threadId);
			branches.remove(came_from);

			int size = branches.size();

			if (size > 1)
			{
				// Mark this junction as visited by BT so TB can find it
				this->setDirectionRouteBT(at, go_to);
				return Junction(at, came_from, branches);
			}
			else if (size == 0)
			{
				return Junction(at, came_from, branches);
			}

			go_to = branches.getNext();
			at = at.move(go_to);
			came_from = reverseDir(go_to);
		}
	}

	void walkThread_DFS_TB(int threadID, std::vector<Direction>*& pTB, Position& posOverlap, std::atomic<bool>& foundSolution)
	{
		std::vector<Junction> pJunctionStack;
		pJunctionStack.reserve(VECTOR_RESERVE_SIZE);

		pJunctionStack.push_back(firstJunction_TB(pMaze->getStart(), threadID));

		while (!(pJunctionStack.empty()) && /*!foundSolution*/!foundSolution.load(std::memory_order_relaxed))
		{
			Junction& junc = pJunctionStack.back();

			if (junc.at == pMaze->getEnd())
			{
				bool expected = false;
				// Atomic check: if false, set to true. Only the winner enters.
				//if (foundSolution.compare_exchange_strong(expected, true))
				if (foundSolution.compare_exchange_strong(expected, true, std::memory_order_release, std::memory_order_relaxed))
				{
					this->ReconstructPathTBForDFS(pTB, pJunctionStack, junc.at);
				}
				return;
			}

			if (junc.isOverlap)
			{
				//{
				//	std::lock_guard<std::mutex> lock(mtx);
				//	posOverlap = junc.at;
				//}

				posOverlap = junc.at;

				bool expected = false;
				//if (foundSolution.compare_exchange_strong(expected, true))
				if (foundSolution.compare_exchange_strong(expected, true, std::memory_order_release, std::memory_order_relaxed))
				{
					this->ReconstructPathTBForDFS(pTB, pJunctionStack, junc.at);
				}
				return;
			}

			//----------------------------------------------------
		    // Compute next direction (or discover deadend)
		    //----------------------------------------------------
			Direction dir;

			if (junc.isDeadend())
			{
				dir = Direction::Uninitialized;
			}
			else
			{
				dir = junc.pBranches.getNextThreads(junc.at, this->pMaze);
			}

			//----------------------------------------------------
			// If no valid direction, backtrack
			//----------------------------------------------------
			if (dir == Direction::Uninitialized)
			{
				pJunctionStack.pop_back();

				if (!pJunctionStack.empty())
				{
					Junction& parent = pJunctionStack.back();
					parent.pBranches.popCurrThreads(parent.at, this->pMaze);
				}
				continue;
			}

			//----------------------------------------------------
			// Otherwise follow the path and push next junction
			//----------------------------------------------------
			pJunctionStack.push_back(followPath_TB(junc.at, dir, threadID));
		}
	}

	void walkThread_DFS_BT(int threadID, std::vector<Junction>& pBTStack, Position& posOverlap, std::atomic<bool>& foundSolution, std::atomic<bool>& foundOverlap)
	{
		std::vector<Junction> pJunctionStack;
		pJunctionStack.reserve(VECTOR_RESERVE_SIZE);

		pJunctionStack.push_back(firstJunction_BT(pMaze->getEnd(), threadID));

		// Run until solution found (by TB) or stack empty
		while (!(pJunctionStack.empty()) && /*!foundSolution*/!foundSolution.load(std::memory_order_relaxed))
		{
			Junction& junc = pJunctionStack.back();

			//----------------------------------------------------
			// Compute next direction (or discover deadend)
			//----------------------------------------------------
			Direction dir;

			if (junc.isDeadend())
			{
				dir = Direction::Uninitialized;
			}
			else
			{
				dir = junc.pBranches.getNextThreads(junc.at, this->pMaze);
			}

			//----------------------------------------------------
			// If no valid direction, backtrack
			//----------------------------------------------------
			if (dir == Direction::Uninitialized)
			{
				pJunctionStack.pop_back();

				if (!pJunctionStack.empty())
				{
					Junction& parent = pJunctionStack.back();
					parent.pBranches.popCurrThreads(parent.at, this->pMaze);
				}
				continue;
			}

			//----------------------------------------------------
			// Otherwise follow the path and push next junction
			//----------------------------------------------------
			pJunctionStack.push_back(followPath_BT(junc.at, dir, threadID));
		}

		//----------------------------------------------------
		// Post-Process: Recover the BT half of the path
		//----------------------------------------------------

		// 1. Get the overlap position found by TB
		Position pos = posOverlap;
		//{
		//	std::lock_guard<std::mutex> lock(mtx);
		//	pos = posOverlap;
		//}

		// If no overlap was set (TB finished alone, or no path found), exit.
		if (pos == Position(-1, -1))
		{
			return;
		}

		//printf("pos ( %d , %d)\n", pos.row, pos.col);

		// 2. Unwind stack to find the connection point
		while (!(pJunctionStack.empty()) && /*!foundOverlap*/!foundOverlap.load(std::memory_order_relaxed))
		{
			Junction& junc = pJunctionStack.back();

			if (junc.at != pos)
			{
				pJunctionStack.pop_back();
			}
			else
			{
				bool expected = false;
				//if (foundOverlap.compare_exchange_strong(expected, true))
				if (foundOverlap.compare_exchange_strong(expected, true,
					std::memory_order_release,
					std::memory_order_relaxed))
				{
					pBTStack = std::move(pJunctionStack);
				}
				return;
			}
		}
	}

	void ReconstructPathTBForDFS(std::vector<Direction>*& fullPath, std::vector<Junction>& junctionStack, Position& posEnd)
	{
		fullPath->clear(); // Ensure clean slate
		fullPath->reserve(VECTOR_RESERVE_SIZE);

		Position curr = pMaze->getStart();
		Direction go_to = Direction::Uninitialized;
		Direction came_from = Direction::Uninitialized;

		// Iterator to read the choices made by the winning thread
		auto iter = junctionStack.begin();

		while (curr != posEnd)
		{
			Branches pBranches = pMaze->getBranches(curr, 0);

			if (Direction::Uninitialized != came_from)
			{
				pBranches.remove(came_from);
			}

			if (pBranches.size() == 1)
			{
				// Deterministic path (corridor), no stack lookup needed
				go_to = pBranches.getNext();
			}
			else if (pBranches.size() > 1)
			{
				// Junction: Consult the winner's stack
				if (iter == junctionStack.end())
				{
					printf("Error in solution--Ran out of junction choices prematurely.\n");
					assert(false);
				}

				// Re-execute the decision stored in the stack
				go_to = iter->pBranches.getCurr();
				iter++;
			}
			else if (pBranches.size() == 0)
			{
				printf("Error in solution--leads to deadend.\n");
				assert(false);
			}

			fullPath->push_back(go_to);
			curr = curr.move(go_to);
			came_from = reverseDir(go_to);
		}
	}

	void ReconstructPathBTForDFS(std::vector<Direction>*& fullPath, std::vector<Junction>& junctionStack)
	{
		// Safety check first
		if (junctionStack.empty()) return;

		// We traverse the BT stack in reverse
		auto iter = junctionStack.rbegin();

		// The start of this walk is the Overlap position (top of the BT stack)
		Position curr = iter->at;

		Position posEnd = pMaze->getEnd();
		Direction came_from = Direction::Uninitialized;
		Direction go_to = Direction::Uninitialized;

		while (curr != posEnd)
		{
			// 1. Check Stack Synchronization FIRST (High Priority)
			// If our current physical location matches the node recorded in the history,
			// we MUST take the instruction from history, no matter the geometry.
			if (/*iter != junctionStack.rend() && */curr == iter->at)
			{
				go_to = iter->from;

				// Move the history iterator forward
				iter++;
			}
			// 2. If not in stack, use Geometry (Corridor Logic)
			else
			{
				Branches pBranches = pMaze->getBranches(curr, 0);

				if (came_from != Direction::Uninitialized)
				{
					pBranches.remove(came_from);
				}

				if (pBranches.size() == 1)
				{
					go_to = pBranches.getNext();
				}
				else
				{
					// If we are here, we have >1 branches but we are NOT in the stack,
					// OR we have 0 branches (Dead end). Both are fatal errors.
					// This means we walked off the correct path.
					printf("Error: Reconstruct Logic Desync at (%d, %d). Size: %d\n", curr.row, curr.col, pBranches.size());
					assert(false);
				}
			}

			// 3. Execute Move
			fullPath->push_back(go_to);
			curr = curr.move(go_to);
			came_from = reverseDir(go_to);
		}
	}

	void walkThreadTB(std::vector<Direction>*& pSolve, bool& exit, bool& firstExit)
	{
		pSolve->reserve(VECTOR_RESERVE_SIZE); // Optimized allocations...

		// Get full solution path.
		Position curr = pMaze->getStart();
		Position target = pMaze->getEnd();
		Position overlap;
		Direction go_to = Direction::Uninitialized;
		Direction came_from = Direction::Uninitialized;

		while (!(curr == target) && !firstExit)
		{
			if (getDirectionRouteBT(curr) != Direction::Uninitialized)
			{
				break;
			}

			ListDirection pMoves = pMaze->getMoves(curr);

			if (Direction::Uninitialized != came_from)
			{
				pMoves.remove(came_from);
			}

			if (pMoves.size() == 1)
			{
				go_to = pMoves.front();
			}
			else if (pMoves.size() > 1)
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(1));
				continue;
			}
			else if (pMoves.size() == 0)
			{
				printf("Error in solution--leads to deadend.");
				assert(false);
			}

			pSolve->push_back(go_to);
			curr = curr.move(go_to);
			overlap = curr;
			//pMaze->setOverlap(curr);
			came_from = reverseDir(go_to);
		}

		exit = true;

		// Reconstruct path
		if (firstExit)
		{
			curr = pMaze->getStart();
			pSolve->clear();
			while (curr != target)
			{
				Direction dir = getDirectionRouteBT(curr);
				pSolve->push_back(dir);
				curr = curr.move(dir);
			}
		}
		else
		{
			curr = overlap;
			while (curr != target)
			{
				Direction dir = getDirectionRouteBT(curr);
				pSolve->push_back(dir);
				curr = curr.move(dir);
			}
		}
	}

	void walkThread_BFS_BT(bool& exit, bool& firstExit)
	{
		Position start = pMaze->getEnd();
		Position end = pMaze->getStart();
		std::queue<Position> q;
		q.push(start);

		// Mark start route (no parent)
		setDirectionRouteBT(start, Direction::Uninitialized);

		while (!q.empty() && !exit)
		{
			Position cur = q.front();
			q.pop();

			if (pMaze->isDeadCell(cur))
			{
				continue;
			}

			if (cur == end)
			{
				firstExit = true;
				break;
			}

			Direction came_from = getDirectionRouteBT(cur);

			if (came_from != Direction::South && pMaze->canMove(cur, Direction::South))
			{
				Position nextPos = cur.move(Direction::South);

				if (getDirectionRouteBT(nextPos) == Direction::Uninitialized)
				{
					// Mark parent
					setDirectionRouteBT(nextPos, Direction::North);
					q.push(nextPos);
				}
			}

			if (came_from != Direction::West && pMaze->canMove(cur, Direction::West))
			{
				Position nextPos = cur.move(Direction::West);

				if (getDirectionRouteBT(nextPos) == Direction::Uninitialized)
				{
					// Mark parent
					setDirectionRouteBT(nextPos, Direction::East);
					q.push(nextPos);
				}
			}

			if (came_from != Direction::East && pMaze->canMove(cur, Direction::East))
			{
				Position nextPos = cur.move(Direction::East);

				if (getDirectionRouteBT(nextPos) == Direction::Uninitialized)
				{
					// Mark parent
					setDirectionRouteBT(nextPos, Direction::West);
					q.push(nextPos);
				}
			}

			if (came_from != Direction::North && pMaze->canMove(cur, Direction::North))
			{
				Position nextPos = cur.move(Direction::North);

				if (getDirectionRouteBT(nextPos) == Direction::Uninitialized)
				{
					// Mark parent
					setDirectionRouteBT(nextPos, Direction::South);
					q.push(nextPos);
				}
			}

			std::this_thread::yield();
		}
	}

	void PruneDeadCellsHeadChunk(int N, bool& exit, CircularData& outQue, CircularData& inQue)
	{
		int chunk = pMaze->height / N;
		int remainder = pMaze->height % N;

		int rowStart = 0;
		int rowEnd = chunk + (remainder > 0 ? 1 : 0);

		std::vector<Position> stackDFS;
		stackDFS.reserve(2024);


		// Step 1: find all initial degree-1 cells
		for (int r = rowStart; r < rowEnd; ++r)
		{
			for (int c = 0; c < pMaze->width; ++c)
			{
				Position pos(r, c);

				if (pos == pMaze->getStart())
				{
					continue;
				}

				ListDirection moves = pMaze->getMoves(pos);
				if (moves.size() <= 1)
				{
					stackDFS.push_back(pos);
				}
			}
		}

		while (!exit)
		{
			// Step 2: BFS to mark dead cells
			while (!stackDFS.empty())
			{
				Position pos = stackDFS.back();
				stackDFS.pop_back();

				if (pMaze->isDeadCell(pos))
				{
					continue;
				}
				// Mark as dead
				pMaze->setDead(pos);

				// Check neighbors
				ListDirection neighborChoices = pMaze->getMoves(pos);

				Direction dir = neighborChoices.front(); // only one or zero neighbor Choice

				if (Direction::Uninitialized != dir)
				{
					Position neighborPos = pos.move(dir);

					// deactivate dead cell
					switch (dir)
					{
					case Direction::North:
					{
						pMaze->setSouth(neighborPos);
						break;
					}
					case Direction::South:
					{
						pMaze->setSouth(pos);
						break;
					}
					case Direction::East:
					{
						pMaze->setEast(pos);
						break;
					}
					case Direction::West:
					{
						pMaze->setEast(neighborPos);
						break;
					}
					case Direction::Uninitialized:
					default:
						assert(false);
						break;
					}

					if (neighborPos == pMaze->getStart())
					{
						continue;
					}


					ListDirection neighborMoves = pMaze->getMoves(neighborPos);

					if (neighborMoves.size() <= 1)
					{
						if (neighborPos.row < rowEnd)
						{
							//queBFS.push(neighborPos);
							stackDFS.push_back(neighborPos);
						}
						else
						{
							outQue.PushBack(neighborPos);
						}
					}
				}
			}

			// step 3: Mark the dead cells coming from other chunk
			Position pPos;
			while (inQue.PopFront(pPos))
			{
				stackDFS.push_back(pPos);
			}

			std::this_thread::yield();
		}
	}

	void PruneDeadCellsTailChunk(int N, bool& exit, CircularData& outQue, CircularData& inQue)
	{
		int chunk = pMaze->height / N;
		int remainder = pMaze->height % N;

		int rowStart = (N - 1) * chunk + std::min(N - 1, remainder);
		int rowEnd = pMaze->height;

		std::vector<Position> stackDFS;
		stackDFS.reserve(2024);

		// Step 1: find all initial degree-1 cells
		for (int r = rowStart; r < rowEnd; ++r)
		{
			for (int c = 0; c < pMaze->width; ++c)
			{
				Position pos(r, c);

				if (pos == pMaze->getEnd())
				{
					continue;
				}

				ListDirection moves = pMaze->getMoves(pos);
				if (moves.size() <= 1)
				{
					stackDFS.push_back(pos);
				}
			}
		}

		while (!exit)
		{
			// Step 2: BFS to mark dead cells
			while (!stackDFS.empty())
			{
				Position pos = stackDFS.back();
				stackDFS.pop_back();

				if (pMaze->isDeadCell(pos))
				{
					continue; // already dead
				}
				// Mark as dead
				pMaze->setDead(pos);

				// Check neighbors
				ListDirection neighborChoices = pMaze->getMoves(pos);

				Direction dir = neighborChoices.front(); // only one or zero neighbor Choice

				if (Direction::Uninitialized != dir)
				{
					Position neighborPos = pos.move(dir);

					// deactivate dead cell
					switch (dir)
					{
					case Direction::North:
					{
						pMaze->setSouth(neighborPos);
						break;
					}
					case Direction::South:
					{
						pMaze->setSouth(pos);
						break;
					}
					case Direction::East:
					{
						pMaze->setEast(pos);
						break;
					}
					case Direction::West:
					{
						pMaze->setEast(neighborPos);
						break;
					}
					case Direction::Uninitialized:
					default:
						assert(false);
						break;
					}

					if (neighborPos == pMaze->getEnd())
					{
						continue;;
					}

					ListDirection neighborMoves = pMaze->getMoves(neighborPos);

					if (neighborMoves.size() <= 1)
					{
						if (neighborPos.row < rowStart)
						{
							outQue.PushBack(neighborPos);
						}
						else
						{
							stackDFS.push_back(neighborPos);
						}
					}
				}
			}

			// step 3: Mark the dead cells coming from other chunk
			Position pPos;
			while (inQue.PopFront(pPos))
			{
				stackDFS.push_back(pPos);
			}

			std::this_thread::yield();
		}
	}

	void PruneDeadCellsMiddleChunk(int N, int thdID, bool& exit, CircularData& outQueTop, CircularData& outQueBottom, CircularData& inQue)
	{  
		int chunk = pMaze->height / N;
 		int remainder = pMaze->height % N;

		int rowStart = thdID * chunk + std::min(thdID, remainder);
		int rowEnd = (thdID + 1) * chunk + std::min(thdID + 1, remainder);

		std::vector<Position> stackDFS;
		stackDFS.reserve(2024);

		// Step 1: find all initial degree-1 cells
		for (int r = rowStart; r < rowEnd; ++r)
		{
			for (int c = 0; c < pMaze->width; ++c)
			{
				Position pos(r, c);

				ListDirection moves = pMaze->getMoves(pos);
				if (moves.size() <= 1)
				{
					stackDFS.push_back(pos);
				}
			}
		}

		while (!exit)
		{
			// Step 2: BFS to mark dead cells
			while (!stackDFS.empty())
			{
				Position pos = stackDFS.back();
				stackDFS.pop_back();

				if (pMaze->isDeadCell(pos))
				{
					continue; // already dead
				}
				// Mark as dead
				pMaze->setDead(pos);

				// Check neighbors
				ListDirection neighborChoices = pMaze->getMoves(pos);

				Direction dir = neighborChoices.front(); // only one or zero neighbor Choice

				if (Direction::Uninitialized != dir)
				{
					Position neighborPos = pos.move(dir);

					// deactivate dead cell
					switch (dir)
					{
					case Direction::North:
					{
						pMaze->setSouth(neighborPos);
						break;
					}
					case Direction::South:
					{
						pMaze->setSouth(pos);
						break;
					}
					case Direction::East:
					{
						pMaze->setEast(pos);
						break;
					}
					case Direction::West:
					{
						pMaze->setEast(neighborPos);
						break;
					}
					case Direction::Uninitialized:
					default:
						assert(false);
						break;
					}

					ListDirection neighborMoves = pMaze->getMoves(neighborPos);

					if (neighborMoves.size() <= 1)
					{
						if (neighborPos.row < rowStart)
						{
							outQueTop.PushBack(neighborPos);
						}
						else if (neighborPos.row >= rowEnd)
						{
							outQueBottom.PushBack(neighborPos);
						}
						else
						{
							stackDFS.push_back(neighborPos);
						}
					}
				}
			}

			// step 3: Mark the dead cells coming from other chunk
			Position pPos;
			while (inQue.PopFront(pPos))
			{
				stackDFS.push_back(pPos);
			}

			std::this_thread::yield();
		}
	}

	void StartParallelPruning_Method1(int N, std::vector<Direction>*& pTB)
	{
		bool exit = false;
		bool firstExit = false;

		std::vector<CircularData> queues((size_t)N);
		std::vector<std::thread> midThreads;

		std::thread headThread(&MTMazeStudentSolver::PruneDeadCellsHeadChunk,
			this,
			N,
			std::ref(exit),
			std::ref(queues[1]),
			std::ref(queues[0]));

		std::thread tailThread(&MTMazeStudentSolver::PruneDeadCellsTailChunk,
			this,
			N,
			std::ref(exit),
			std::ref(queues[(size_t)N - 2]),
			std::ref(queues[(size_t)N - 1]));

		for (int thdID = 1; thdID < N - 1; thdID++)
		{
			midThreads.emplace_back(&MTMazeStudentSolver::PruneDeadCellsMiddleChunk,
				this,
				N,
				thdID,
				std::ref(exit),
				std::ref(queues[(size_t)thdID - 1]), // inQue from previous
				std::ref(queues[(size_t)thdID + 1]), // outQue to next
				std::ref(queues[(size_t)thdID]));    // local queue
		}
		
		std::thread walkReverse(&MTMazeStudentSolver::walkThread_BFS_BT, this, std::ref(exit), std::ref(firstExit));

		this->walkThreadTB(pTB, exit, firstExit);

		headThread.join();
		tailThread.join();
		for (auto& t : midThreads)
		{
			t.join();
		}

		walkReverse.join();
	}

	void StartParallelDFSWalking_Method2(int TBN, int BTN, std::vector<Direction>*& pTB)
	{
		// 1. Shared State Initialization
		std::atomic<bool> foundSolution(false); // Flags if ANY thread reaches goal/overlap
		std::atomic<bool> foundOverlap(false);  // Flags specifically if BT connects to TB
		//std::mutex mtx;                         // Protects posOverlap
		Position posOverlap(-1, -1);            // The meeting point coordinates

		std::vector<Junction> pBTStack;         // To store the winning BT path stack
		std::vector<std::thread> threads;

		// 2. Launch Top-Bottom (TB) Threads
	    // These threads fill 'pTB' directly if they win
		for (int i = 0; i < TBN; ++i)
		{
			threads.emplace_back(&MTMazeStudentSolver::walkThread_DFS_TB, this,
				i, std::ref(pTB), std::ref(posOverlap), std::ref(foundSolution));

			std::this_thread::sleep_for(std::chrono::microseconds(1));
		}

		// 3. Launch Bottom-Top (BT) Threads
	    // These threads fill 'pBTStack' if they connect
	    // Note: We offset the threadID by TBN so they have unique indices in the Branch structs
		for (int i = 0; i < BTN; ++i)
		{
			threads.emplace_back(&MTMazeStudentSolver::walkThread_DFS_BT, this,
				i + TBN, std::ref(pBTStack), std::ref(posOverlap), std::ref(foundSolution), std::ref(foundOverlap));

			std::this_thread::sleep_for(std::chrono::microseconds(1));
		}

		// 4. Wait for completion
		for (auto& t : threads)
		{
			if (t.joinable()) t.join();
		}

		if (foundOverlap.load())
		{
			this->ReconstructPathBTForDFS(pTB, pBTStack);
		}

	}

	std::vector<Direction> *Solve() override
	{	
		std::vector<Direction>* pTB = new std::vector<Direction>();

		//this->StartParallelPruning_Method1(24, pTB);
		this->StartParallelDFSWalking_Method2(10, 10, pTB);
		return pTB;
	}

	std::vector<Direction>* Solve_1()
	{
		std::vector<Direction>* pTB = new std::vector<Direction>();
		this->StartParallelPruning_Method1(168, pTB);
		return pTB;
	}

	std::vector<Direction>* Solve_2()
	{
		std::vector<Direction>* pTB = new std::vector<Direction>();
		this->StartParallelDFSWalking_Method2(40, 40, pTB);
		return pTB;
	}
};

#endif

// --- End of File ----
