//------------------------------------------------------
// Copyright 2025, Ed Keenan, all rights reserved.
//------------------------------------------------------

#ifndef MT_Maze_Student_Solver_H
#define MT_Maze_Student_Solver_H

#include <list>
#include <vector>

#include "Maze.h"
#include "MazeSolver.h"
#include "Direction.h"


// Feel free to change your class any way you see fit
// It needs to inherit at some point from the MazeSolver
class MTMazeStudentSolver : public SkippingMazeSolver //public MazeSolver 
{
public: 
	MTMazeStudentSolver(Maze *maze) 
	//: MazeSolver( maze )
		: SkippingMazeSolver(maze)
	{
		assert(pMaze);
	}

	MTMazeStudentSolver() = delete;
	MTMazeStudentSolver(const MTMazeStudentSolver &) = delete;
	MTMazeStudentSolver &operator=(const MTMazeStudentSolver &) = delete;
	~MTMazeStudentSolver() = default;


	void setDirectionRouteTB(Position at, Direction from)
	{
		unsigned int val = pMaze->getCell(at);
		int dir = 0;

		switch (from)
		{
		case Direction::East:
			dir = 0x10000000;
			break;
		case Direction::West:
			dir = 0x20000000;
			break;
		case Direction::South:
			dir = 0x40000000;
			break;
		case Direction::North:
			dir = 0x80000000;
			break;
		case Direction::Uninitialized:
		default:
			//assert(0);
			break;
		}

		val |= dir;
		pMaze->setCell(at, val);
	}

	Direction getDirectionRouteTB(Position at)
	{
		Direction dir = Direction::Uninitialized;
		unsigned int val = pMaze->getCell(at);
		val &= 0xF0000000;

		switch (val)
		{
		case 0x10000000:
			dir = Direction::East;
			break;
		case 0x20000000:
			dir = Direction::West;
			break;
		case 0x40000000:
			dir = Direction::South;
			break;
		case 0x80000000:
			dir = Direction::North;
			break;
		default:
			//assert(0);
			break;
		}

		return dir;
	}

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

		val |= dir;                 // set only the BT direction bits
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








	Choice firstJunction1(Position pos)
	{
		ListDirection Moves = pMaze->getMoves(pos);

		if (Moves.size() == 1)
		{
			Direction tmp = Moves.begin();
			return followPath(pos, tmp);
		}
		else
		{
			Choice p(pos, Direction::Uninitialized, Moves);
			return p;
		}
	}

	Choice followPath1(Position at, Direction dir)
	{
		ListDirection Choices;
		Direction go_to = dir;
		Direction came_from = reverseDir(dir);
		at = at.move(go_to);

		do
		{
			if (at == (pMaze->getEnd()))
			{
				//Choices = pMaze->getMoves(at);
				//Choices.remove(came_from);
				return Choice(at, came_from, Choices);
			}

			Choices = pMaze->getMoves(at);
			Choices.remove(came_from);

			if (Choices.size() == 1)
			{
				go_to = Choices.begin();
				at = at.move(go_to);
				came_from = reverseDir(go_to);
			}

		} while (Choices.size() == 1);

		Choice pRet(at, came_from, Choices);

		return pRet;
	}

	void walkThread_DFS(std::vector<Direction>*& pSolve, bool& exit)
	{
		std::vector<Choice> pChoiceStack;
		pChoiceStack.reserve(VECTOR_RESERVE_SIZE);

		Choice ch;

		pChoiceStack.push_back(firstJunction(pMaze->getStart()));

		while (!(pChoiceStack.size() == 0))
		{
			ch = pChoiceStack.back();

			if (ch.at == pMaze->getEnd())
			{
				//pChoiceStack.pop_back();
				Trace::out("Got Solution\n");
				break; // solution found
			}

			if (ch.isDeadend())
			{
				// backtrack.
				pChoiceStack.pop_back();

				if (!(pChoiceStack.size() == 0))
				{
					pChoiceStack.back().pChoices.pop_front();
				}

				continue;
			}

			pChoiceStack.push_back(followPath(ch.at, ch.pChoices.front()));

		}

		exit = true;

		std::vector<Choice>::iterator iter = pChoiceStack.begin();
		pSolve->reserve(VECTOR_RESERVE_SIZE);  // Optimized allocations...

		// Get full solution path.
		Position curr = pMaze->getStart();
		Direction go_to = Direction::Uninitialized;
		Direction came_from = Direction::Uninitialized;

		while (!(curr == pMaze->getEnd()))
		{
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
				if (iter == pChoiceStack.end())
				{
					printf("Error in solution--Ran out of junction choices prematurely.\n");
					assert(false);
				}

				go_to = iter->pChoices.front();
				iter++;
			}
			else if (pMoves.size() == 0)
			{
				printf("Error in solution--leads to deadend.");
				assert(false);
			}

			pSolve->push_back(go_to);
			curr = curr.move(go_to);
			came_from = reverseDir(go_to);
		}
	}







	Choice firstJunction(Position pos)
	{
		ListDirection Moves = pMaze->getMoves(pos);

		if (Moves.size() == 1)
		{
			Direction tmp = Moves.begin();
			return followPath(pos, tmp);
		}
		else
		{
			Choice p(pos, Direction::Uninitialized, Moves);
			return p;
		}
	}

	Choice followPath(Position at, Direction dir)
	{
		ListDirection Choices;
		Direction go_to = dir;
		Direction came_from = reverseDir(dir);

		if (!pMaze->canMove(at, go_to))
		{
			return Choice(at, came_from, Choices, true);
		}

		// move to the first position
		at = at.move(go_to);

		while (true)
		{
			// Check if we reached the start or end
			if (at == pMaze->getEnd())
			{
				Choices = pMaze->getMoves(at);
				Choices.remove(came_from);
				return Choice(at, came_from, Choices);
			}

			// DEAD cell with mark
			if (pMaze->isDeadCell(at))
			{
				return Choice(at, came_from, Choices, true);
			}

			// 3) Determine possible moves (excluding where we came from)
			Choices = pMaze->getMoves(at);
			Choices.remove(came_from);

			if (Choices.size() == 0)
			{
				// Dead cell without/with mark
				return Choice(at, came_from, Choices, true);
			}
			else if (Choices.size() == 1)
			{
				// Corridor — continue straight
				go_to = Choices.begin();
				at = at.move(go_to);
				came_from = reverseDir(go_to);
				continue; // keep moving
			}
			else
			{
				// choice node
				return Choice(at, came_from, Choices);
			}
		}
	}

	void walkThread_s(std::vector<Direction>*& pSolve, bool& exit)
	{
		std::vector<Choice> pChoiceStack;
		pChoiceStack.reserve(VECTOR_RESERVE_SIZE);

		pChoiceStack.push_back(firstJunction(pMaze->getStart()));

		Choice ch;

		while (!(pChoiceStack.size() == 0))
		{
			ch = pChoiceStack.back();

			if (ch.at == pMaze->getEnd())
			{
				pChoiceStack.pop_back();
				Trace::out("Got Solution\n");
				break; // solution found
			}

			if (ch.isDeadCell || pMaze->isDeadCell(ch.at))
			{
				// backtrack.
				pChoiceStack.pop_back();

				if (!(pChoiceStack.size() == 0))
				{
					pChoiceStack.back().pChoices.pop_front();
				}

				continue;
			}

			if (ch.isDeadend())
			{
				// backtrack.
				pChoiceStack.pop_back();

				if (!(pChoiceStack.size() == 0))
				{
					pChoiceStack.back().pChoices.pop_front();
				}

				continue;
			}

			// found a valid path which is not blocked

			Direction dir = ch.pChoices.front();
			pChoiceStack.push_back(followPath(ch.at, dir));
		}


		std::vector<Choice>::iterator iter = pChoiceStack.begin();
		pSolve->reserve(VECTOR_RESERVE_SIZE);  // Optimized allocations...

		exit = true;

		// Get full solution path.
		Position curr = pMaze->getStart();
		Direction go_to = Direction::Uninitialized;
		Direction came_from = Direction::Uninitialized;

		while (!(curr == pMaze->getEnd()))
		{
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
				go_to = iter++->pChoices.front();
			}
			else if (pMoves.size() == 0)
			{
				printf("Error in solution--leads to deadend.");
				assert(false);
			}

			pSolve->push_back(go_to);
			curr = curr.move(go_to);
			came_from = reverseDir(go_to);
		}
	}














	void walkThreadTB(std::vector<Direction>*& pSolve, bool& exit)
	{
		DWORD_PTR mask = (1ULL << 2);

		HANDLE thread = GetCurrentThread();
		DWORD_PTR result = SetThreadAffinityMask(thread, mask);

		if (result == 0)
		{
			printf("Failed to set affinity\n");
		}

		pSolve->reserve(VECTOR_RESERVE_SIZE); // Optimized allocations...

		// Get full solution path.
		Position curr = pMaze->getStart();
		Position target = pMaze->getEnd();
		Position overlap;
		Direction go_to = Direction::Uninitialized;
		Direction came_from = Direction::Uninitialized;

		while (!(curr == target)/* && !exit*/)
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
			pMaze->setOverlap(curr);
			came_from = reverseDir(go_to);

			//std::this_thread::yield();
		}

		exit = true;

		// Reconstruct path
		curr = overlap;
		while (curr != target)
		{
			Direction dir = getDirectionRouteBT(curr);
			pSolve->push_back(dir);
			curr = curr.move(dir);
		}
	}


	void walkThread_BFS_TB(std::vector<Direction>*& pSolve, bool& exit)
	{
		bool found = false;
		int count = 0;
		Position start = pMaze->getStart();
		Position end = pMaze->getEnd();
		std::queue<Position> q;

		q.push(start);
		//Position overlap;

		// Mark start route (no parent)
		setDirectionRouteTB(start, Direction::Uninitialized);

		while (!q.empty())
		{
			Position cur = q.front();
			q.pop();
			count++;

			if (cur == end)
			{
				exit = true;
				found = true;
				break;
			}

			if (pMaze->isDeadCell(cur))
			{
				continue;
			}

			Direction came_from = getDirectionRouteTB(cur);

			if (came_from != Direction::South && pMaze->canMove(cur, Direction::South))
			{
				Position nextPos = cur.move(Direction::South);

				if (getDirectionRouteTB(nextPos) == Direction::Uninitialized)
				{
					// Mark parent
					setDirectionRouteTB(nextPos, Direction::North);
					q.push(nextPos);
				}
			}

			if(came_from != Direction::West && pMaze->canMove(cur, Direction::West))
			{
				Position nextPos = cur.move(Direction::West);

				if (getDirectionRouteTB(nextPos) == Direction::Uninitialized)
				{
					// Mark parent
					setDirectionRouteTB(nextPos, Direction::East);
					q.push(nextPos);
				}
			}

			if (came_from != Direction::East && pMaze->canMove(cur, Direction::East))
			{
				Position nextPos = cur.move(Direction::East);

				if (getDirectionRouteTB(nextPos) == Direction::Uninitialized)
				{
					// Mark parent
					setDirectionRouteTB(nextPos, Direction::West);
					q.push(nextPos);
				}
			}

			if (came_from != Direction::North && pMaze->canMove(cur, Direction::North))
			{
				Position nextPos = cur.move(Direction::North);

				if (getDirectionRouteTB(nextPos) == Direction::Uninitialized)
				{
					// Mark parent
					setDirectionRouteTB(nextPos, Direction::South);
					q.push(nextPos);
				}
			}

			std::this_thread::yield();
		}

		Trace::out("Count1: %d\n", count);

		assert(found);
		exit = true;

		pSolve->reserve(VECTOR_RESERVE_SIZE);
		// Reconstruct path
		Position curr = end;
		while (curr != start)
		{
			Direction dir = getDirectionRouteTB(curr);
			pSolve->push_back(reverseDir(dir));
			curr = curr.move(dir);
		}

		std::reverse(pSolve->begin(), pSolve->end());
	}

	void walkThread_BFS_BT(bool& exit)
	{

		DWORD_PTR mask = (1ULL << 1);

		HANDLE thread = GetCurrentThread();
		DWORD_PTR result = SetThreadAffinityMask(thread, mask);

		if (result == 0)
		{
			printf("Failed to set affinity\n");
		}


		bool found = false;
		Position start = pMaze->getEnd();
		Position end = pMaze->getStart();
		std::queue<Position> q;
		q.push(start);
		int count = 0;

		// Mark start route (no parent)
		setDirectionRouteBT(start, Direction::Uninitialized);

		while (!q.empty() && !exit)
		{
			Position cur = q.front();
			q.pop();
			count++;

			if (pMaze->isDeadCell(cur))
			{
				continue;
			}

			if (cur == end)
			{
				found = true;
				break;
			}

			ListDirection moves = pMaze->getMoves(cur);
	
			moves.remove(getDirectionRouteBT(cur));

			Direction dir = moves.front();
			while (dir != Direction::Uninitialized)
			{
				Position nextPos = cur.move(dir);

				if (getDirectionRouteBT(nextPos) != Direction::Uninitialized)
				{
					moves.pop_front();
					dir = moves.front();
					continue; // already visited
				}

				setDirectionRouteBT(nextPos, reverseDir(dir));

				q.push(nextPos);

				moves.pop_front();
				dir = moves.front();
			}

			std::this_thread::yield();
		}

		Trace::out("Count2: %d\n", count);
	}

	void LaunchPruningThreads(int N, std::vector<Direction>*& pTB)
	{
		bool exit = false;


		std::vector<CircularData> queues((size_t)N);
		std::vector<std::thread> midThreads;

		std::thread headThread(&Maze::PruneDeadCellsHeadChunk,
			pMaze,
			N,
			std::ref(exit),
			std::ref(queues[1]),
			std::ref(queues[0]));

		for (int thdID = 1; thdID < N - 1; thdID++)
		{
			midThreads.emplace_back(&Maze::PruneDeadCellsMiddleChunk,
				pMaze,
				N,
				thdID,
				std::ref(exit),
				std::ref(queues[(size_t)thdID - 1]), // inQue from previous
				std::ref(queues[(size_t)thdID + 1]), // outQue to next
				std::ref(queues[(size_t)thdID]));    // local queue
		}

		std::thread tailThread(&Maze::PruneDeadCellsTailChunk,
			pMaze,
			N,
			std::ref(exit),
			std::ref(queues[(size_t)N - 2]),
			std::ref(queues[(size_t)N - 1]));
		
		
		std::thread walk1(&MTMazeStudentSolver::walkThread_BFS_BT,
			this,
			std::ref(exit)
		);

		//std::thread walk2(&MTMazeStudentSolver::walkThread_DFS,
		//	this,
		//	std::ref(pTB),
		//	std::ref(exit)
		//);


		this->/*walkThread_DFS*//*walkThread_s*/walkThreadTB(pTB, exit);

		headThread.join();
		tailThread.join();
		for (auto& t : midThreads)
		{
			t.join();
		}


		walk1.join();
		//walk2.join();
	}

	std::vector<Direction> *Solve() override
	{
		// Do your magic here

		std::vector<Direction>* pTB = new std::vector<Direction>();

		this->LaunchPruningThreads(8, pTB);

		return pTB;
	}
};

#endif

// --- End of File ----
