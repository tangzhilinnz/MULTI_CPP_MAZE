#ifndef ST_Maze_Solver_BFS_H
#define ST_Maze_Solver_BFS_H

// Algorithm & Maze generation - refactored and modified from material on the internet
//    Influenced by many different solvers and maze categories
//    Purpose of the maze is to create a concurrent problem set for C++ concurrency class
//    Maze solving algorithm and generators were derived from University of Maryland - Java Development

#include <list>
#include <vector>
#include <queue>
#include <algorithm>

#include "Direction.h"
#include "Choice.h"
#include "SkippingMazeSolver.h"


class STMazeSolverBFS: public SkippingMazeSolver 
{
public:	
		
	STMazeSolverBFS(Maze *maze) 
	: SkippingMazeSolver( maze )
	{
		exploring = Direction::Uninitialized;
		assert( maze );
	}


	STMazeSolverBFS() = delete;
	STMazeSolverBFS(const STMazeSolverBFS &) = delete;
	STMazeSolverBFS &operator=(const STMazeSolverBFS &) = delete;
	~STMazeSolverBFS() = default;

	bool expandBFS( std::queue<Choice> &queueList, Choice &pNode ) 
	{
		// Nothing dynamic 
		ListDirection ds = pNode.pChoices;
		bool result = false;
		Choice pTmp;

		if( ds.south == Direction::South )
		{	
			result = followBFS( pTmp, pNode.at, Direction::South );
			if( result == true )
			{
				pNode = pTmp;
				return true;
			}
			setDirectionRoute( pTmp.at, pTmp.from );
			queueList.push( pTmp );
		}
		if( ds.east == Direction::East )
		{		
			result = followBFS( pTmp, pNode.at, Direction::East );
			if( result == true )
			{
				pNode = pTmp;
				return true;
			}
			setDirectionRoute( pTmp.at, pTmp.from );
			queueList.push( pTmp );
		}
		if( ds.west == Direction::West)
		{			
			result = followBFS( pTmp, pNode.at, Direction::West );
			if( result == true )
			{
				pNode = pTmp;
				return true;
			}
			setDirectionRoute( pTmp.at, pTmp.from );
			queueList.push( pTmp );
		}
		if( ds.north == Direction::North )
		{		
			result = followBFS( pTmp, pNode.at, Direction::North );
			if( result == true )
			{
				pNode = pTmp;
				return true;
			}
			setDirectionRoute( pTmp.at, pTmp.from );
			queueList.push( pTmp );
		}

		return result;
	}

	bool followBFS( Choice &pRet, Position at, Direction dir )  //throws SolutionFoundSkip() 
	{
		bool result = false;
		ListDirection Choices;
		Direction go_to = dir;
		Direction came_from = reverseDir(dir);
		at = at.move(go_to);

		do 
		{
			if( at == (pMaze->getEnd()) )
			{
				pRet.Set( at, reverseDir(go_to), Choices ) ;
				return true;
			//	throw SolutionFoundSkip( at, reverseDir(go_to) );
			}
			if( at == (pMaze->getStart()) )
			{
				pRet.Set( at, reverseDir(go_to), Choices ) ;
				return true;
			//	throw SolutionFoundSkip( at, reverseDir(go_to) );
			}

			Choices = pMaze->getMoves(at);
			Choices.remove(came_from);
	
			if(Choices.size() == 1) 
			{
				go_to = Choices.begin();
				at = at.move(go_to);
				came_from = reverseDir(go_to);
			}

		} while( Choices.size() == 1 );
		
		pRet.Set( at, came_from, Choices ) ;

		return result;
	}


	std::vector<Direction> *Solve() 
	{	
		std::queue<Choice> queueList;
		Choice Node;

		// Start from the 1st location
		Choice pTmp( firstChoicePreRoute(pMaze->getStart()) );
		queueList.push(pTmp);

		while(queueList.size()) 
		{
			Node = queueList.front();
			{
				// Node is not a deadend, do a deeper search
				if( !Node.isDeadend() ) 
				{
					if( expandBFS( queueList, Node ) )
						{
							SolutionFoundSkip e(Node.at,Node.from);
							return FindRoute( e );
						};
				}
				else
				{
					// Do not add - deadend
					// not contributing to the solution
				}
			}
			queueList.pop();
		}  

		// No solution found.
		return 0;
	}

	std::vector<Direction> *FindRoute(SolutionFoundSkip e) 
	{
		//printf("\n\n --------soln------------\n");
		std::vector<Direction> *soln = new std::vector<Direction>();
		soln->reserve(VECTOR_RESERVE_SIZE);  // Optimized allocations...

		// Start at End
		Choice pChoice;
		pChoice = followRoute(soln, e.pos, e.from);
			
		// Get the next choice
		while( !(pChoice.at == pMaze->getStart())  )
		{
			pChoice = followRoute( soln, pChoice.at, getDirectionRoute(pChoice.at)  );
		}
		
		std::reverse(soln->begin(), soln->end());
		return soln;
	}


	void setDirectionRoute( Position at, Direction from )
	{
		unsigned int val = pMaze->getCell( at );
		int dir = 0;

		switch(from)
		{
			case Direction::East:
				dir =  0x10000000;
				break;
			case Direction::West:
				dir =  0x20000000;
				break;
			case Direction::South:
				dir =  0x40000000;
				break;
			case Direction::North:
				dir =  0x80000000;
				break;
			case Direction::Uninitialized:
			default:
				assert(0);
				break;
		}

		val |= dir;
		pMaze->setCell(at, val);
	}

	Direction getDirectionRoute( Position at )
	{
		Direction dir = Direction::Uninitialized;
		unsigned int val = pMaze->getCell( at );
		val &= 0xF0000000;

		switch(val)
		{
			case 0x10000000:
				dir =  Direction::East;
				break;
			case 0x20000000:
				dir =  Direction::West;
				break;
			case 0x40000000:
				dir =  Direction::South;
				break;
			case 0x80000000:
				dir =  Direction::North;
				break;
			default:
				assert(0);
				break;
		}

		return dir;
	}

	Choice followRoute( std::vector<Direction> *soln, Position at, Direction dir )  
	{
		ListDirection Choices;
		Direction go_to = dir;
		Direction came_from = reverseDir(dir);
		at = at.move(go_to);

		do 
		{
			if( at == (pMaze->getStart()) )
			{
				soln->push_back(came_from);

				Choice pRet( at, came_from, Choices ) ;
				return pRet;
			}

			Choices = pMaze->getMoves(at);
			Choices.remove(came_from);
	
			soln->push_back(came_from);

			if(Choices.size() == 1) 
			{
				go_to = Choices.begin();
				at = at.move(go_to);
				came_from = reverseDir(go_to);
			}

		} while( Choices.size() == 1 );
		
		Choice pRet( at, came_from, Choices ) ;

		return pRet;
	}

	Choice followPreRoute(Position at, Direction dir )   
	{
		ListDirection Choices;
		Direction go_to = dir;
		Direction came_from = reverseDir(dir);
		at = at.move(go_to);

		do 
		{
			if( at == (pMaze->getEnd()) )
			{
				assert(0);
				
			}
			if( at == (pMaze->getStart()) )
			{
				assert(0);
			}

			Choices = pMaze->getMoves(at);
			Choices.remove(came_from);

			setDirectionRoute( at, came_from );

			if(Choices.size() == 1) 
			{
				go_to = Choices.begin();
				at = at.move(go_to);
				came_from = reverseDir(go_to);
			}

		} while( Choices.size() == 1 );
		
		Choice pRet( at, came_from, Choices ) ;

		return pRet;
	}

	Choice firstChoicePreRoute( Position pos ) 
	{
		ListDirection Moves = pMaze->getMoves(pos);

		if(Moves.size() == 1)
		{
			Direction tmp = Moves.begin();
			return followPreRoute(pos, tmp);
		}
		else
		{
			Choice p(pos, Direction::Uninitialized, Moves);
			return p;		
		}
	}
	
	private:
		Direction exploring;
		char pad[4];
};

#endif

// --- End of File ----
