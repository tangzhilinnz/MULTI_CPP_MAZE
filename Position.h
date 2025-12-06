#ifndef POSITION_MAZE_H
#define POSITION_MAZE_H

#include "Direction.h"
#include "assert.h"

class Position 
{
public:
	Position()
	{
		this->row = 0;
		this->col = 0;
	}

	Position(const Position &) = default;
	Position &operator = (const Position &) = default;
	~Position() = default;

	Position(int row, int col) 
	{
		this->row = row;
		this->col = col;
	}

	bool operator ==( const Position &p ) const 
	{
		return ( (this->row == p.row) && (this->col == p.col));
	}

	bool operator !=(const Position& p) const
	{
		return !(*this == p);
	}

	Position move(Direction dir) 
	{
		Position tmp;

		switch(dir) 
		{
		case Direction::North:
			tmp = Position(row-1,col);
			break;

		case Direction::South:			
			tmp = Position(row+1,col);
			break;

		case Direction::East:
			tmp = Position(row,col+1);
			break;

		case Direction::West:
			tmp = Position(row,col-1);
			break;

		case Direction::Uninitialized:
		default:
			assert(false);
			break;
		}

		return tmp;
	}
		
	public: 
		// data
		int row;
		int col;
};

#endif

// --- End of File ----
