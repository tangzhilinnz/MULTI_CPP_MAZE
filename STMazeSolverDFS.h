#ifndef ST_Maze_Solver_DFS_H
#define ST_Maze_Solver_DFS_H

// Algorithm & Maze generation - refactored and modified from material on the internet
//    Influenced by many different solvers and maze categories
//    Purpose of the maze is to create a concurrent problem set for C++ concurrency class
//    Maze solving algorithm and generators were derived from University of Maryland - Java Development

#include <list>
#include <vector>

#include "Choice.h"
#include "SkippingMazeSolver.h"

class STMazeSolverDFS : public SkippingMazeSolver 
{
public: 
	STMazeSolverDFS(Maze *maze) 
	: SkippingMazeSolver( maze )
	{
		assert(pMaze);
	}

	STMazeSolverDFS() = delete;
	STMazeSolverDFS(const STMazeSolverDFS &) = delete;
	STMazeSolverDFS &operator=(const STMazeSolverDFS &) = delete;
	~STMazeSolverDFS() = default;

	std::vector<Direction> *Solve() override
	{
		std::vector<Choice> pChoiceStack;
		pChoiceStack.reserve(VECTOR_RESERVE_SIZE);  // Optimized allocations...

		Choice ch;
		try 
		{
			pChoiceStack.push_back(firstChoice(pMaze->getStart()));

			while(!(pChoiceStack.size() == 0) )
			{
				ch = pChoiceStack.back();
				if(ch.isDeadend()) 
				{
					// backtrack.
					pChoiceStack.pop_back();

					if(!(pChoiceStack.size() == 0) )
					{
						pChoiceStack.back().pChoices.pop_front();
					}

					continue;
				}
				
				pChoiceStack.push_back(follow(ch.at, ch.pChoices.front()));
				
			}
			// No solution found.
			return 0;
		}
		catch (SolutionFoundSkip& e) 
		{
			std::vector<Choice>::iterator iter = pChoiceStack.begin();
			std::vector<Direction> *pFullPath = new std::vector<Direction>();
			pFullPath->reserve(VECTOR_RESERVE_SIZE);  // Optimized allocations...

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

				pFullPath->push_back(go_to);
				curr = curr.move(go_to);
				came_from = reverseDir(go_to);
			}

			return pFullPath;
		}
	}
};

#endif

// --- End of File ----
