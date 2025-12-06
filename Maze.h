#ifndef MAZE_H
#define MAZE_H

#include <atomic>
#include <list>
#include <memory>

#include "Position.h"
#include "Direction.h"
#include "CircularData.h"

struct ListDirection;
struct Branches;

#define DEBUG_PRINT 0

// Maze cells are (row,col) starting at (0,0)
// Linear storage of integers 0 to (width*height - 1)
// Upper Upper corner (0,0) or index 0
// Lower Left corner (width-1, height-1) or (width*height - 1)

class Maze
{
public:

	Maze();
	Maze(const Maze &) = delete;
	Maze &operator = (const Maze &) = delete;
	~Maze();

	Maze( int _width, int _height);

	void Load( const char * const fileName );

	ListDirection getMoves(Position pos);
	Branches getBranches(Position pos, int threadId);
	bool canMove( Position pos, Direction dir );
	bool checkSolution( std::vector<Direction> &soln );

	Position getStart();
	Position getEnd() ;;

	void setEast(Position pos);
	void setSouth(Position pos);
	unsigned int getCell(Position pos);
	void setCell(Position pos, unsigned int val);

	bool isOverlapCell(Position pos);
	void setOverlap(Position pos);

	bool isDeadCell(Position pos);
	void setDead(Position pos);

	bool isBranchOccupied(Position pos);
	void setBranchOccupied(Position pos);

	bool isBranchDead(Position pos);
	void setBranchDead(Position pos);

	bool checkBranchOccupied(Position pos, Direction dir);
	bool checkBranchDead(Position pos, Direction dir);

// data:
	std::atomic_uint *poMazeData;
	int height;
	int width;
	int solvable;
	char pad[4];
};

#endif

// --- End of File ----
