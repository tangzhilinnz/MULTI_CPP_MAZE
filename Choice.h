#ifndef CHOICE_H
#define CHOICE_H

#include <list>
#include <deque>
#include <memory>
#include <assert.h>

#include "Position.h"
#include "Maze.h"

struct ListDirection
{

	ListDirection()
		:south(Direction::Uninitialized),
		east(Direction::Uninitialized),
		west(Direction::Uninitialized),
		north(Direction::Uninitialized)
	{
	}

	int size()
	{
		int count = 0;

		if (this->south != Direction::Uninitialized)
		{
			count++;
		}
		if (this->east != Direction::Uninitialized)
		{
			count++;
		}
		if (this->west != Direction::Uninitialized)
		{
			count++;
		}
		if (this->north != Direction::Uninitialized)
		{
			count++;
		}

		return count;
	}

	void remove( Direction dir)
	{
		if( this->south == dir )
			this->south = Direction::Uninitialized;

		if( this->east == dir )
			this->east = Direction::Uninitialized;

		if( this->west == dir )
			this->west = Direction::Uninitialized;

		if( this->north == dir )
			this->north =  Direction::Uninitialized;
	}

	Direction getNext( Direction dir )
	{
		Direction retDir = Direction::Uninitialized;

		switch(dir)
		{
			case Direction::South:

				if( this->east == Direction::East )
				{
					retDir = Direction::East;
					break;
				}
				if( this->west == Direction::West )
				{
					retDir = Direction::West;
					break;
				}
				if( this->north == Direction::North)
				{
					retDir = Direction::North;
					break;
				}
				break;

			case Direction::East:
				if( this->west == Direction::West )
				{
					retDir = Direction::West;
					break;
				}
				if( this->north == Direction::North)
				{
					retDir = Direction::North;
					break;
				}
				break;

			case Direction::West:
				if( this->north == Direction::North)
				{
					retDir = Direction::North;
					break;
				}
				break;


			case Direction::North:
				break;
			
			case Direction::Uninitialized:
			default:
				assert(0);
				break;

		}

		return retDir;
	}

	Direction begin ()
	{
		Direction retDir = Direction::Uninitialized;

		if( this->south == Direction::South )
			return Direction::South;

		if( this->east == Direction::East )
			return Direction::East;

		if( this->west == Direction::West)
			return Direction::West;

		if( this->north == Direction::North )
			return Direction::North;

		return retDir;
	}

	Direction front()
	{
		Direction retDir = Direction::Uninitialized;

		if( this->south == Direction::South )
			return Direction::South;

		if( this->east == Direction::East )
			return Direction::East;

		if( this->west == Direction::West)
			return Direction::West;

		if( this->north == Direction::North )
			return Direction::North;

		return retDir;
	}

	Direction pop_front()
	{	
		if( this->south == Direction::South )
		{
			this->south = Direction::Uninitialized;
			return Direction::South ;
		}

		if( this->east == Direction::East )
		{	
			this->east = Direction::Uninitialized;
			return Direction::East;
		}

		if( this->west == Direction::West)
		{
			this->west = Direction::Uninitialized;
			return Direction::West;
		}

		if( this->north == Direction::North )
		{	
			this->north = Direction::Uninitialized;
			return Direction::North; 
		}

	assert(0);
	return Direction::Uninitialized;
	}	
	
	// Data ----------------------------------------------------
	Direction south;
	Direction east;
	Direction west;
	Direction north;
};

class Choice 
{

public:
	Choice()
		: isDeadCell(false),
		at(),
		from(),
		pChoices()
	{
		this->from = Direction::Uninitialized;
	}

	Choice(const Choice &) = default;
	Choice &operator = (const Choice &) = default;

	~Choice()
	{
	}

	Choice(Position _at, Direction _from,  ListDirection _pChoices, bool _isDeadCell = false) 
	{
		this->isDeadCell = _isDeadCell;
		this->at = _at;
		this->pChoices = _pChoices;
		this->from = _from;
	}

	void Set( Position _at, Direction _from,  ListDirection _pChoices ) 
	{
		this->at = _at;
		this->pChoices = _pChoices;
		this->from = _from;
	}


	
	bool isDeadend() 
	{
		return (pChoices.size() == 0);
	}

// data:

	bool isDeadCell;
	char pad[3];
	Position at;
	Direction from;
	ListDirection pChoices;

};

struct Branches
{
	Branches()
		: index(0), count(0)
	{
		directions[(int)Direction::North] = Direction::Uninitialized;
		directions[(int)Direction::East] = Direction::Uninitialized;
		directions[(int)Direction::South] = Direction::Uninitialized;
		directions[(int)Direction::West] = Direction::Uninitialized;
	}

	Branches(int thdID)
		: index(thdID & 3), count(0)
	{
		directions[(int)Direction::North] = Direction::Uninitialized;
		directions[(int)Direction::East] = Direction::Uninitialized;
		directions[(int)Direction::South] = Direction::Uninitialized;
		directions[(int)Direction::West] = Direction::Uninitialized;
	}

	Branches(const Branches& rhs)
		: index(rhs.index),
		count(rhs.count)
	{
		directions[0] = rhs.directions[0];
		directions[1] = rhs.directions[1];
		directions[2] = rhs.directions[2];
		directions[3] = rhs.directions[3];
	}

	Branches& operator = (const Branches& rhs)
	{
		if (this == &rhs)
			return *this;

		this->index = rhs.index;
		this->count = rhs.count;

		this->directions[0] = rhs.directions[0];
		this->directions[1] = rhs.directions[1];
		this->directions[2] = rhs.directions[2];
		this->directions[3] = rhs.directions[3];

		return *this;
	}

	~Branches() = default;

	int size()
	{
		return count;
	}

	void remove(Direction dir)
	{
		assert(dir != Direction::Uninitialized);

		int i = (int)dir;

		// already removed, no change
		if (directions[i] == Direction::Uninitialized)
		{
			return;
		}

		directions[i] = Direction::Uninitialized;
		--count;
	}

	void add(Direction dir)
	{
		assert(dir != Direction::Uninitialized);

		int i = (int)dir;

		// already present, no change
		if (directions[i] != Direction::Uninitialized)
		{
			return;
		}

		directions[i] = dir;
		++count;
	}

	Direction getNext()
	{	
		if (count == 0)
			return Direction::Uninitialized;

		// Try up to 4 entries
		for (int i = 0; i < 4; ++i)
		{
			index = (index + 1) & 3;

			if (directions[index] != Direction::Uninitialized)
			{
				return directions[index];
			}
		}

		// Should never reach here if count > 0
		assert(false);
		return Direction::Uninitialized;
	}

	Direction getNextThreads(Position& at, Maze* pMaze)
	{
		if (count == 0)
		{
			return Direction::Uninitialized;
		}

		int fallbackIndex = -1;

		// Check only the other 3 directions
		for (int attempt = 0; attempt < 4; ++attempt)
		{
			index = (index + 1) & 3;
			Direction dir = directions[index];

			// this slot already removed or invalid
			if (dir == Direction::Uninitialized)
			{
				continue;
			}

			// If dead, remove permanently
			if (pMaze->checkBranchDead(at, dir))
			{
				directions[index] = Direction::Uninitialized;
				--count;
				if (count == 0)
				{
					return Direction::Uninitialized;
				}
				continue;
			}

			// Only now we know: dir is alive (not dead, not uninitialized)
			if (fallbackIndex == -1)
			{
				fallbackIndex = index;
			}

			// If occupied, skip but DO NOT remove it
			if (pMaze->checkBranchOccupied(at, dir))
			{
				continue;
			}

			// Found a free, non-dead and non-occupied branch
			pMaze->setBranchOccupied(at.move(dir));
			// Note: 'index' is currently correct here because we break the loop immediately
			return dir;
		}

		// If we are using the fallback, we MUST restore the index
		// so that popCurrThreads looks at the correct slot later.
		if (fallbackIndex != -1)
		{
			index = fallbackIndex;
			return directions[index];
		}

		return Direction::Uninitialized;
	}

	Direction popCurrThreads(Position& at, Maze* pMaze)
	{
		Direction dir = directions[index];
		assert(dir != Direction::Uninitialized);

		//if (dir == Direction::Uninitialized)
		//{
		//	return dir; // safe fallback
		//}

		pMaze->setBranchDead(at.move(dir));

		directions[index] = Direction::Uninitialized;
		count--;
		return dir;
	}


	Direction getCurr()
	{
		return directions[index];
	}

	Direction popCurr()
	{
		Direction dir = directions[index];
		assert(dir != Direction::Uninitialized);

		if (dir == Direction::Uninitialized)
		{
			return dir; // safe fallback
		}

		directions[index] = Direction::Uninitialized;
		count--;
		return dir;
	}

	// Data ----------------------------------------------------
	Direction directions[4];
	int index;
	int count;
};

class Junction
{

public:
	Junction()
		: at(),
		pBranches()
	{
		this->from = Direction::Uninitialized;
	}

	Junction(const Junction& rhs)
		: isOverlap(rhs.isOverlap),
		at(rhs.at),
		from(rhs.from),
		pBranches(rhs.pBranches)
	{
	}

	Junction& operator = (const Junction& rhs)
	{
		if (this == &rhs)
			return *this;

		this->isOverlap = rhs.isOverlap;

		this->at = rhs.at;
		this->from = rhs.from;
		this->pBranches = rhs.pBranches;

		return *this;
	}

	~Junction()
	{
	}

	Junction(Position _at, Direction _from, Branches _pBranches, bool _isOverlap = false)
		: isOverlap(_isOverlap),
		at(_at),
		from(_from),
		pBranches(_pBranches)
	{
	}

	void Set(Position _at, Direction _from, Branches _pBranches, bool _isOverlap)
	{
		this->at = _at;
		this->pBranches = _pBranches;
		this->from = _from;
		this->isOverlap = _isOverlap;
	}

	bool isDeadend()
	{
		return (this->pBranches.size() == 0);
	}

	// data:

	bool isOverlap;
	char pad[3];
	Position at;
	Direction from;
	Branches pBranches;

};

#endif

// --- End of File ----
