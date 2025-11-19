//------------------------------------------------------
// Copyright 2025, Ed Keenan, all rights reserved.
//------------------------------------------------------

// Algorithm & Maze generation - refactored and modified from material on the internet
//    Influenced by many different solvers and maze categories
//    Purpose of the maze is to create a concurrent problem set for C++ concurrency class
//    Maze solving algorithm and generators were derived from University of Maryland - Java Development

#include <atomic>
#include <array>
#include <vector>
#include <memory>
#include <queue>

#include "Maze.h"
#include "File.h"
#include "Choice.h"

using namespace Azul;

enum class InternalBit
{
	EAST_BIT = 1,
	SOUTH_BIT = 2,
	DEAD_BIT = 4,
	OVERLAP_BIT = 8,
};

struct FileHeader
{
	int width;
	int height;
	int solvable;
	// int data[];  data following
};

Maze::Maze()
{
	this->width = 0;
	this->height = 0;
	this->poMazeData = nullptr;
}

Maze::~Maze()
{
	delete this->poMazeData;
}

Maze::Maze( int _width, int _height)
{
	this->width = _width;
	this->height = _height;
	unsigned int totalSize = (unsigned int)(this->width * this->height);
	this->poMazeData = new std::atomic_uint[ totalSize ];
	assert( poMazeData );
}

void Maze::Load( const char * const inFileName )
{
	File::Handle fh;
	File::Error  ferror;

	//----------- Open to Read  --------------------------------
	ferror = File::Open(fh, inFileName, File::Mode::READ );
	assert( ferror == File::Error::SUCCESS);

	//----------- Seek ------------------------------------------

	ferror = File::Seek( fh, File::Position::END, 0 );
	assert( ferror == File::Error::SUCCESS);

	DWORD fileSize;
	ferror = File::Tell( fh, fileSize );
	assert( ferror == File::Error::SUCCESS);

	unsigned char *pBuff = new unsigned char[fileSize];
	assert( pBuff != 0 );
	unsigned char *pBuff_Original = pBuff;
	
	ferror = File::Seek( fh, File::Position::BEGIN, 0 );
	assert( ferror == File::Error::SUCCESS);

	ferror = File::Read( fh, pBuff, fileSize );
	assert( ferror == File::Error::SUCCESS);

	ferror = File::Close( fh );
	assert( ferror == File::Error::SUCCESS );

	// ----- Everything is in memory ------
	// --- Now decompress it 

	FileHeader *pHdr;

	pHdr = (FileHeader *) pBuff;

		// copy it to Maze data
		this->height = pHdr->height;
		this->width = pHdr->width;
		this->solvable = pHdr->solvable;

	pBuff += sizeof(FileHeader);

	int *pIntData = (int *)pBuff;

	// reserve the space
	unsigned int totalSize = (unsigned int)(this->width * this->height);
	this->poMazeData = new std::atomic_uint[ totalSize ];
	assert(this->poMazeData );
	memset(this->poMazeData, 0x0, (this->width * this->height) * sizeof(unsigned int) );

	Position pos = Position(0,0);
	while(pos.row < height) 
	{
		pos = Position(pos.row, 0);
		while(pos.col < width) 
		{
			int bits = *pIntData++;

			for( int bit = 0; (bit < 16 && pos.col < width); bit++) 
			{
				if((bits & 1) == 1)
				{
					setEast(pos); 
				}

				if((bits & 2) == 2)
				{
					setSouth(pos);
				}

				bits >>= 2;
				
				pos = pos.move(Direction::East);
			}
		}
		pos = pos.move(Direction::South);
	}

	delete pBuff_Original;
}
 

ListDirection Maze::getMoves(Position pos) 
{
	ListDirection result;
		
	if( canMove(pos,Direction::South) )
	{
		result.south = Direction::South;
	}
	if(canMove(pos,Direction::East))
	{
		result.east = Direction::East;
	}
	if(canMove(pos,Direction::West))
	{
		result.west = Direction::West;
	}
	if(canMove(pos,Direction::North))
	{
		result.north = Direction::North;
	}

	return result;
}
	
bool Maze::canMove(Position pos, Direction dir)
{
	bool status = false;

	switch(dir) 
	{
	case Direction::North:
		if(pos.row == 0) 
		{
			status = false;
		}
		else 
		{
			status = ((getCell(pos.move(Direction::North)) & (int)InternalBit::SOUTH_BIT) == 0);
		}
		break;

	case Direction::South:
		status = ((getCell(pos) & (int)InternalBit::SOUTH_BIT) == 0);
		break;

	case Direction::East:
		status = ((getCell(pos) & (int)InternalBit::EAST_BIT) == 0);
		break;

	case Direction::West:
		if(pos.col == 0) 
		{
			status = false;
		}
		else 
		{
			status = ((getCell(pos.move(Direction::West)) & (int)InternalBit::EAST_BIT) == 0);
		}	
		break;

	case Direction::Uninitialized:
	default:
		assert( false );
		break;
	}

	return status;
		
}

Position Maze::getStart() 
{
	Position tmp = Position(0, this->width/2);
	return tmp;
}


Position Maze::getEnd() 
{
	Position tmp = Position(this->height-1, this->width / 2);
	return tmp;
}

void Maze::setEast(Position pos) 
{
	unsigned int newVal = getCell(pos) | (unsigned int)InternalBit::EAST_BIT;
	setCell(pos,newVal);
}	

void Maze::setSouth(Position pos) 
{
	unsigned int newVal = getCell(pos) | (unsigned int)InternalBit::SOUTH_BIT;
	setCell(pos,newVal);
}

unsigned int Maze::getCell(Position pos) 
{
	unsigned int val = poMazeData[pos.row * this->width + pos.col];
	return val;
}
	
void Maze::setCell(Position pos, unsigned int val) 
{
	this->poMazeData[pos.row * this->width + pos.col] = val;
}

bool Maze::checkSolution(std::vector<Direction> &soln)
{
	assert( &soln );

	bool results = true;	
	Position at = getStart();

	for(auto iter = begin(soln); iter != end(soln); ++iter ) 
	{
		Direction dir = *iter;


		if( !canMove(at,dir) )
		{
			results = false;
			break;
		}

		at = at.move(dir);
	}

	if ( !(at == getEnd()) )
	{
		results = false;
	}

	if( results )
	{
		Trace::out2("    checkSolution(%d elements): passed\n",(int)soln.size());
	}
	else
	{
		Trace::out2("    checkSolution(%d elements): FAILED!! <<<<-------\n",(int)soln.size());
	}

	return results;
}


void Maze::PruneDeadCellsHeadChunk(int N, bool& exit, CircularData& outQue, CircularData& inQue)
{
	int chunk = height / N;
	int remainder = height % N;

	int rowStart = 0;
	int rowEnd = chunk + (remainder > 0 ? 1 : 0);

	//std::queue<Position> queBFS;
	std::vector<Position> stackDFS;
	stackDFS.reserve(2024);


	// Step 1: find all initial degree-1 cells
	for (int r = rowStart; r < rowEnd; ++r)
	{
		for (int c = 0; c < this->width; ++c)
		{
			Position pos(r, c);

			if (pos == this->getStart())
			{
				continue;
			}

			ListDirection moves = getMoves(pos);
			if (moves.size() <= 1)
			{
				//queBFS.push(pos);
				stackDFS.push_back(pos);
			}
		}
	}

	while (!exit)
	{
		// Step 2: BFS to mark dead cells
		while (/*!queBFS.empty()*/!stackDFS.empty())
		{
			//Position pos = queBFS.front();
			//queBFS.pop();
			Position pos = stackDFS.back();
			stackDFS.pop_back();

			if (this->isDeadCell(pos))
			{
				continue; // already dead
			}
			// Mark as dead
			this->setDead(pos);

			// Check neighbors
			ListDirection neighborChoices = getMoves(pos);

			Direction dir = neighborChoices.front(); // only one or zero neighbor Choice

			if (Direction::Uninitialized != dir)
			{
				Position neighborPos = pos.move(dir);

				// deactivate dead cell
				switch (dir)
				{
				case Direction::North:
				{
					this->setSouth(neighborPos);
					break;
				}
				case Direction::South:
				{
					this->setSouth(pos);
					break;
				}
				case Direction::East:
				{
					this->setEast(pos);
					break;
				}
				case Direction::West:
				{
					this->setEast(neighborPos);
					break;
				}
				case Direction::Uninitialized:
				default:
					assert(false);
					break;
				}

				if (neighborPos == this->getStart())
				{
					continue;
				}


				ListDirection neighborMoves = getMoves(neighborPos);

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
			//queBFS.push(pPos);
			stackDFS.push_back(pPos);
		}

		std::this_thread::yield();
	}

	//Trace::out("Thread head exit() ======================================= \n");
}

void Maze::PruneDeadCellsTailChunk(int N, bool& exit, CircularData& outQue, CircularData& inQue)
{
	int chunk = height / N;
	int remainder = height % N;

	int rowStart = (N - 1) * chunk + std::min(N - 1, remainder);
	int rowEnd = height;

	//std::queue<Position> queBFS;
	std::vector<Position> stackDFS;
	stackDFS.reserve(2024);

	// Step 1: find all initial degree-1 cells
	for (int r = rowStart; r < rowEnd; ++r)
	{
		for (int c = 0; c < this->width; ++c)
		{
			Position pos(r, c);

			if (pos == this->getEnd())
			{
				continue;
			}

			ListDirection moves = getMoves(pos);
			if (moves.size() <= 1)
			{
				//queBFS.push(pos);
				stackDFS.push_back(pos);
			}
		}
	}

	while (!exit)
	{
		// Step 2: BFS to mark dead cells
		while (/*!queBFS.empty()*/!stackDFS.empty())
		{
			//Position pos = queBFS.front();
			//queBFS.pop();
			Position pos = stackDFS.back();
			stackDFS.pop_back();

			if (this->isDeadCell(pos))
			{
				continue; // already dead
			}
			// Mark as dead
			this->setDead(pos);

			// Check neighbors
			ListDirection neighborChoices = getMoves(pos);

			Direction dir = neighborChoices.front(); // only one or zero neighbor Choice

			if (Direction::Uninitialized != dir)
			{
				Position neighborPos = pos.move(dir);

				// deactivate dead cell
				switch (dir)
				{
				case Direction::North:
				{
					this->setSouth(neighborPos);
					break;
				}
				case Direction::South:
				{
					this->setSouth(pos);
					break;
				}
				case Direction::East:
				{
					this->setEast(pos);
					break;
				}
				case Direction::West:
				{
					this->setEast(neighborPos);
					break;
				}
				case Direction::Uninitialized:
				default:
					assert(false);
					break;
				}

				if (neighborPos == this->getEnd())
				{
					continue;;
				}

				ListDirection neighborMoves = getMoves(neighborPos);

				if (neighborMoves.size() <= 1)
				{
					if (neighborPos.row < rowStart)
					{
						outQue.PushBack(neighborPos);
					}
					else
					{
						//queBFS.push(neighborPos);
						stackDFS.push_back(neighborPos);
					}
				}
			}
		}

		// step 3: Mark the dead cells coming from other chunk
		Position pPos;
		while (inQue.PopFront(pPos))
		{
			//queBFS.push(pPos);
			stackDFS.push_back(pPos);
		}

		std::this_thread::yield();
	}



	//Trace::out("Thread tail exit() ======================================= \n");
}

void Maze::PruneDeadCellsChunk(int N, int thdID, bool& exit)
{
	int chunk = height / N;
	int remainder = height % N;

	int rowStart = thdID * chunk + std::min(thdID, remainder);
	int rowEnd = (thdID + 1) * chunk + std::min(thdID + 1, remainder);

	while (!exit)
	{
		// Pass 1: scan my chunk rows
		for (int r = rowStart; r < rowEnd; ++r)
		{
			for (int c = 0; c < width; ++c)
			{
				Position pos(r, c);

				unsigned int cell = getCell(pos);

				if (pos == this->getEnd() || pos == this->getStart())
				{
					continue;
				}

				if (cell & (unsigned int)InternalBit::DEAD_BIT)
				{
					continue;
				}

				ListDirection moves = getMoves(pos);

				if (moves.size() <= 1)
				{
					// mark dead
					cell |= (unsigned int)InternalBit::DEAD_BIT;
					setCell(pos, cell);

					Direction dir = moves.front();

					if (Direction::Uninitialized != dir)
					{
						Position neighborPos = pos.move(dir);

						// deactivate dead cell
						switch (dir)
						{
						case Direction::North:
						{
							this->setSouth(neighborPos);
							break;
						}
						case Direction::South:
						{
							this->setSouth(pos);
							break;
						}
						case Direction::East:
						{
							this->setEast(pos);
							break;
						}
						case Direction::West:
						{
							this->setEast(neighborPos);
							break;
						}
						case Direction::Uninitialized:
						default:
							assert(false);
							break;
						}

					}
				}
			}
		}

		std::this_thread::yield();

	}
}


void Maze::PruneDeadCellsMiddleChunk(int N, int thdID, bool& exit,
	CircularData& outQueTop, CircularData& outQueBottom, CircularData& inQue)
{
	int chunk = height / N;
	int remainder = height % N;

	int rowStart = thdID * chunk + std::min(thdID, remainder);
	int rowEnd = (thdID + 1) * chunk + std::min(thdID + 1, remainder);

	//std::queue<Position> queBFS;
	std::vector<Position> stackDFS;
	stackDFS.reserve(2024);

	// Step 1: find all initial degree-1 cells
	for (int r = rowStart; r < rowEnd; ++r)
	{
		for (int c = 0; c < this->width; ++c)
		{
			Position pos(r, c);

			ListDirection moves = getMoves(pos);
			if (moves.size() <= 1)
			{
				//queBFS.push(pos);
				stackDFS.push_back(pos);
			}
		}
	}

	while (!exit)
	{
		// Step 2: BFS to mark dead cells
		while (/*!queBFS.empty()*/!stackDFS.empty())
		{
			//Position pos = queBFS.front();
			//queBFS.pop();
			Position pos = stackDFS.back();
			stackDFS.pop_back();

			if (this->isDeadCell(pos))
			{
				continue; // already dead
			}
			// Mark as dead
			this->setDead(pos);

			// Check neighbors
			ListDirection neighborChoices = getMoves(pos);

			Direction dir = neighborChoices.front(); // only one or zero neighbor Choice

			if (Direction::Uninitialized != dir)
			{
				Position neighborPos = pos.move(dir);

				// deactivate dead cell
				switch (dir)
				{
				case Direction::North:
				{
					this->setSouth(neighborPos);
					break;
				}
				case Direction::South:
				{
					this->setSouth(pos);
					break;
				}
				case Direction::East:
				{
					this->setEast(pos);
					break;
				}
				case Direction::West:
				{
					this->setEast(neighborPos);
					break;
				}
				case Direction::Uninitialized:
				default:
					assert(false);
					break;
				}

				ListDirection neighborMoves = getMoves(neighborPos);

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
						//queBFS.push(neighborPos);
						stackDFS.push_back(neighborPos);
					}
				}
			}
		}

		// step 3: Mark the dead cells coming from other chunk
		Position pPos;
		while (inQue.PopFront(pPos))
		{
			//queBFS.push(pPos);
			stackDFS.push_back(pPos);
		}

		std::this_thread::yield();
	}

	//Trace::out("thread %d exit() ======================================= \n", thdID);
}


bool Maze::isDeadCell(Position pos)
{
	return (this->poMazeData[pos.row * this->width + pos.col].load() &
		(unsigned int)InternalBit::DEAD_BIT) != 0;
}

void Maze::setDead(Position pos)
{
	this->poMazeData[pos.row * this->width + pos.col].fetch_or((unsigned int)InternalBit::DEAD_BIT);
}

bool Maze::isOverlapCell(Position pos)
{
	return (this->poMazeData[pos.row * this->width + pos.col].load() &
		(unsigned int)InternalBit::OVERLAP_BIT) != 0;
}

void Maze::setOverlap(Position pos)
{
	this->poMazeData[pos.row * this->width + pos.col].fetch_or((unsigned int)InternalBit::OVERLAP_BIT);
}

//--- End of File ------------------------------

