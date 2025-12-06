#ifndef Skipping_Maze_Solver_H
#define Skipping_Maze_Solver_H

// Algorithm & Maze generation - refactored and modified from material on the internet
//    Influenced by many different solvers and maze categories
//    Purpose of the maze is to create a concurrent problem set for C++ concurrency class
//    Maze solving algorithm and generators were derived from University of Maryland - Java Development

#include <exception>
#include "MazeSolver.h"
#include "Position.h"
#include "Choice.h"

#define VECTOR_RESERVE_SIZE 400000

class SolutionFoundSkip : public std::exception
{
public:
		
	SolutionFoundSkip(Position _pos, Direction _from) 
	{
		this->pos = _pos;
		this->from = _from;
	}
		
	Position pos;
	Direction from;
	char pad[4];
};

class SkippingMazeSolver : public MazeSolver 
{
public:
	
	SkippingMazeSolver(Maze *maze) 
	: MazeSolver( maze )
	{
		assert( maze );
	}

	SkippingMazeSolver() = delete;
	SkippingMazeSolver(const SkippingMazeSolver &) = delete;
	SkippingMazeSolver &operator=(const SkippingMazeSolver &) = delete;
	~SkippingMazeSolver() = default;
	
	Choice firstChoice( Position pos ) //throw SolutionFound 
	{
		ListDirection Moves = pMaze->getMoves(pos);

		if(Moves.size() == 1)
		{
			Direction tmp = Moves.begin();
			return follow(pos, tmp);
		}
		else
		{
			Choice p(pos, Direction::Uninitialized, Moves);
			return p;		
		}
	}
	
	Choice follow( Position at, Direction dir )  //throws SolutionFoundSkip() 
	{
		ListDirection Choices;
		Direction go_to = dir;
		Direction came_from = reverseDir(dir);
		at = at.move(go_to);

		do 
		{
			if( at == (pMaze->getEnd()) )
			{
				throw SolutionFoundSkip( at, reverseDir(go_to) );
				
			}
			if( at == (pMaze->getStart()) )
			{
				throw SolutionFoundSkip( at, reverseDir(go_to) );
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
		
		Choice pRet( at, came_from, Choices ) ;

		return pRet;
	}


	void markPath( std::list<Direction> *path) 
	{
		try 
		{
			Choice pChoice = firstChoice( pMaze->getStart() );
				
			Position at = pChoice.at;
			std::list<Direction>::iterator iter = path->begin();//.iterator();
			while(iter != path->end()) 
			{
				pChoice = follow(at, *iter);
				iter++;
				at = pChoice.at;
			} 
		}
		catch(SolutionFoundSkip e) 
		{ 
		}
	}

};

#endif

// --- End of File ----
