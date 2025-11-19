//------------------------------------------------------
// Copyright 2025, Ed Keenan, all rights reserved.
//------------------------------------------------------

#ifndef CHOICE_H
#define CHOICE_H

#include <list>
#include <deque>
#include <memory>

#include "Position.h"

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

		if( this->south != Direction::Uninitialized )
			count++;
		if( this->east != Direction::Uninitialized )
			count++;
		if( this->west != Direction::Uninitialized )
			count++;
		if( this->north != Direction::Uninitialized )
			count++;

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

	Direction frontBT()
	{
		Direction retDir = Direction::Uninitialized;

		if (this->north == Direction::North)
			return Direction::North;

		if (this->west == Direction::West)
			return Direction::West;

		if (this->east == Direction::East)
			return Direction::East;

		if (this->south == Direction::South)
			return Direction::South;

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

#endif

// --- End of File ----
