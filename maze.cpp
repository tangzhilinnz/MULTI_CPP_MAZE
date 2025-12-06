#include <atomic>
#include <array>
#include <vector>
#include <memory>
#include <queue>
#include <fstream>  // REQUIRED FOR LINUX IO
#include <cstring>  // REQUIRED FOR memcpy, memset
#include <cstdio>   // REQUIRED FOR printf
#include <assert.h>  // REQUIRED FOR assert

#include "Maze.h"
//#include "File.h"
#include "Choice.h"

enum class InternalBit
{
	EAST_BIT = 1,
	SOUTH_BIT = 2,
	DEAD_BIT = 4,
	OVERLAP_BIT = 8,

	BRANCH_OCCUPIED_BIT = 16,
	BRANCH_DEAD_BIT = 32,

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
	// --- LINUX/STANDARD C++ IO REPLACEMENT ---

	// 1. Open file using standard ifstream
	std::ifstream file(inFileName, std::ios::binary | std::ios::ate);
	if (!file.is_open())
	{
		printf("Error: Cannot open file %s\n", inFileName);
		assert(false);
		return;
	}

	// 2. Get file size
	std::streamsize fileSize = file.tellg();
	file.seekg(0, std::ios::beg);

	if (fileSize <= 0)
	{
		assert(false);
		return;
	}

	// 3. Read into a vector (Safe memory management)
	std::vector<unsigned char> buffer(fileSize);
	if (!file.read((char*)buffer.data(), fileSize))
	{
		assert(false);
		return;
	}

	// File closes automatically here via destructor, or use file.close()

	// ----- Everything is in memory ------
	// --- Now decompress it 

	unsigned char* pBuff = buffer.data();
	size_t offset = 0;

	// Use memcpy to avoid alignment crashes on non-Windows systems
	FileHeader header;
	assert(fileSize >= sizeof(FileHeader));
	std::memcpy(&header, pBuff, sizeof(FileHeader));

	offset += sizeof(FileHeader);

	// copy it to Maze data
	this->height = header.height;
	this->width = header.width;
	this->solvable = header.solvable;

	int* pIntData = (int*)(pBuff + offset);

	// reserve the space
	unsigned int totalSize = (unsigned int)(this->width * this->height);

	// Safety check against double allocation if Load is called twice
	if (this->poMazeData != nullptr) delete[] this->poMazeData;

	this->poMazeData = new std::atomic_uint[totalSize];
	assert(this->poMazeData);

	// Initialize atomic array safely
	for (unsigned int i = 0; i < totalSize; i++) {
		this->poMazeData[i].store(0, std::memory_order_relaxed);
	}

	Position pos = Position(0, 0);

	// We need to track how many bytes we've read from the int stream
	// Note: pIntData is a pointer to the raw buffer. On x86 Linux this is fine.
	// On strict alignment architectures (ARM), pIntData access might need memcpy.
	// Assuming x86/x64 Linux here.

	while (pos.row < height)
	{
		pos = Position(pos.row, 0);
		while (pos.col < width)
		{
			// Safe read of unaligned integer
			int bits;
			std::memcpy(&bits, pIntData, sizeof(int));
			pIntData++;

			for (int bit = 0; (bit < 16 && pos.col < width); bit++)
			{
				if ((bits & 1) == 1)
				{
					setEast(pos);
				}

				if ((bits & 2) == 2)
				{
					setSouth(pos);
				}

				bits >>= 2;

				pos = pos.move(Direction::East);
			}
		}
		pos = pos.move(Direction::South);
	}
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

Branches Maze::getBranches(Position pos, int threadId)
{
	Branches result(threadId);

	if (canMove(pos, Direction::South))
	{
		result.add(Direction::South);
	}
	if (canMove(pos, Direction::East))
	{
		result.add(Direction::East);
	}
	if (canMove(pos, Direction::West))
	{
		result.add(Direction::West);
	}
	if (canMove(pos, Direction::North))
	{
		result.add(Direction::North);
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
	unsigned int val = poMazeData[pos.row * this->width + pos.col].load(std::memory_order_relaxed);
	//unsigned int val = poMazeData[pos.row * this->width + pos.col].load(std::memory_order_acquire);
	return val;
}
	
void Maze::setCell(Position pos, unsigned int val) 
{
	//this->poMazeData[pos.row * this->width + pos.col] = val;
	this->poMazeData[pos.row * this->width + pos.col].store(val, std::memory_order_relaxed);
	//this->poMazeData[pos.row * this->width + pos.col].store(val, std::memory_order_release);
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
		printf("    checkSolution(%d elements): passed\n",(int)soln.size());
	}
	else
	{
		printf("    checkSolution(%d elements): FAILED!! <<<<-------\n",(int)soln.size());
	}

	return results;
}

bool Maze::isDeadCell(Position pos)
{
	//return (this->poMazeData[pos.row * this->width + pos.col].load() &
	//	(unsigned int)InternalBit::DEAD_BIT) != 0;
	return (this->poMazeData[pos.row * this->width + pos.col].load(std::memory_order_relaxed) &
		(unsigned int)InternalBit::DEAD_BIT) != 0;
	//return (this->poMazeData[pos.row * this->width + pos.col].load(std::memory_order_acquire) &
	//	(unsigned int)InternalBit::DEAD_BIT) != 0;
}

void Maze::setDead(Position pos)
{
	//this->poMazeData[pos.row * this->width + pos.col].fetch_or((unsigned int)InternalBit::DEAD_BIT);
	this->poMazeData[pos.row * this->width + pos.col].fetch_or((unsigned int)InternalBit::DEAD_BIT, std::memory_order_relaxed);
	//this->poMazeData[pos.row * this->width + pos.col].fetch_or((unsigned int)InternalBit::DEAD_BIT, std::memory_order_release);
}

bool Maze::isOverlapCell(Position pos)
{
	//return (this->poMazeData[pos.row * this->width + pos.col].load() &
	//	(unsigned int)InternalBit::OVERLAP_BIT) != 0;
	return (this->poMazeData[pos.row * this->width + pos.col].load(std::memory_order_relaxed) &
		(unsigned int)InternalBit::OVERLAP_BIT) != 0;
	//return (this->poMazeData[pos.row * this->width + pos.col].load(std::memory_order_acquire) &
	//	(unsigned int)InternalBit::OVERLAP_BIT) != 0;
}

void Maze::setOverlap(Position pos)
{
	//this->poMazeData[pos.row * this->width + pos.col].fetch_or((unsigned int)InternalBit::OVERLAP_BIT);
	this->poMazeData[pos.row * this->width + pos.col].fetch_or((unsigned int)InternalBit::OVERLAP_BIT, std::memory_order_relaxed);
	//this->poMazeData[pos.row * this->width + pos.col].fetch_or((unsigned int)InternalBit::OVERLAP_BIT, std::memory_order_release);
}

bool Maze::isBranchOccupied(Position pos)
{
	//return (this->poMazeData[pos.row * this->width + pos.col].load() &
	//	(unsigned int)InternalBit::BRANCH_OCCUPIED_BIT) != 0;
	return (this->poMazeData[pos.row * this->width + pos.col].load(std::memory_order_relaxed) &
		(unsigned int)InternalBit::BRANCH_OCCUPIED_BIT) != 0;
	//return (this->poMazeData[pos.row * this->width + pos.col].load(std::memory_order_acquire) &
	//	(unsigned int)InternalBit::BRANCH_OCCUPIED_BIT) != 0;
}

void Maze::setBranchOccupied(Position pos)
{
	//this->poMazeData[pos.row * this->width + pos.col].fetch_or((unsigned int)InternalBit::BRANCH_OCCUPIED_BIT);
	this->poMazeData[pos.row * this->width + pos.col].fetch_or((unsigned int)InternalBit::BRANCH_OCCUPIED_BIT, std::memory_order_relaxed);
	//this->poMazeData[pos.row * this->width + pos.col].fetch_or((unsigned int)InternalBit::BRANCH_OCCUPIED_BIT, std::memory_order_release);
}

bool Maze::isBranchDead(Position pos)
{
	//return (this->poMazeData[pos.row * this->width + pos.col].load() &
	//	(unsigned int)InternalBit::BRANCH_DEAD_BIT) != 0;
	return (this->poMazeData[pos.row * this->width + pos.col].load(std::memory_order_relaxed) &
		(unsigned int)InternalBit::BRANCH_DEAD_BIT) != 0;
	//return (this->poMazeData[pos.row * this->width + pos.col].load(std::memory_order_acquire) &
	//	(unsigned int)InternalBit::BRANCH_DEAD_BIT) != 0;
}

void Maze::setBranchDead(Position pos)
{
	//this->poMazeData[pos.row * this->width + pos.col].fetch_or((unsigned int)InternalBit::BRANCH_DEAD_BIT);
	this->poMazeData[pos.row * this->width + pos.col].fetch_or((unsigned int)InternalBit::BRANCH_DEAD_BIT, std::memory_order_relaxed);
	//this->poMazeData[pos.row * this->width + pos.col].fetch_or((unsigned int)InternalBit::BRANCH_DEAD_BIT, std::memory_order_release);
}

bool Maze::checkBranchOccupied(Position pos, Direction dir)
{
	assert(dir != Direction::Uninitialized);

	Position next = pos.move(dir);
	return this->isBranchOccupied(next);
}

bool Maze::checkBranchDead(Position pos, Direction dir)
{
	assert(dir != Direction::Uninitialized);

	Position next = pos.move(dir);
	return this->isBranchDead(next);
}


//--- End of File ------------------------------

