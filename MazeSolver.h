//------------------------------------------------------
// Copyright 2025, Ed Keenan, all rights reserved.
//------------------------------------------------------

#ifndef MAZE_SOLVER_H
#define MAZE_SOLVER_H

#include <list>
#include <vector>

#include "Maze.h"

class MazeSolver 
{

public:
	// Constructor, set the maze
	MazeSolver( Maze *maze ) 
	{
		pMaze = maze;
		assert( this->pMaze);
	}	

	MazeSolver() = delete;
	MazeSolver(const MazeSolver &) = delete;
	MazeSolver &operator = (const MazeSolver &) = delete;
	~MazeSolver() = default;

	// Must overload
	virtual std::vector<Direction> *Solve()=0;
	     
protected:
	// data
	Maze *pMaze;

};

#endif

// --- End of File ----
