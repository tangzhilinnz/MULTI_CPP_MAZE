//-----------------------------------------------------------------------------
// Copyright 2025, Ed Keenan, all rights reserved.
//----------------------------------------------------------------------------- 

#include "CircularData.h"
#include <assert.h>

bool CircularData::IsEmpty()
{
	return this->empty;
}

CircularData::CircularData()
	: front(CIRCULAR_DATA_SIZE),
	back(CIRCULAR_DATA_SIZE),
	empty(true),
	full(false)
{
}

bool CircularData::PushBack(Position pPos)
{
	std::lock_guard<std::mutex> lock(this->mtx);

	bool status = true;

	// Do we have space to add?
	if(this->front != this->back || this->empty)
	{
		// add data
		this->data[this->back.Index()] = pPos;
		this->back++;

		this->empty = false;

		// check if this add made it full
		if(this->front == this->back)
		{
			this->full = true;
			printf("full ===================\n");
			// Safety
			assert(false);
		}
	}
	else
	{
		status = false;
	}

	return status;
}

bool CircularData::PopFront(Position& pPos)
{
	std::lock_guard<std::mutex> lock(this->mtx);

	bool status = true;

	// Is there data to process?
	if(this->front != this->back || this->full)
	{
		// Grab one
		pPos = this->data[this->front.Index()];
		this->front++;

		this->full = false;

		// check if this Pop made it Empty
		if(this->front == this->back)
		{
			this->empty = true;
		}
	}
	else
	{
		status = false;
	}
	return status;
}


CircularData::CircularIndex::CircularIndex(unsigned int buffSize)
{

	this->size = buffSize;
	this->mask = (unsigned int)(buffSize - 1);

	// verify that buffSize is a power of 2
	assert((size & (size - 1)) == 0);

	this->index = 0;
}

// postfix
unsigned int CircularData::CircularIndex::operator++(int)
{
	this->index++;

	// Circular 
	this->index = this->index & this->mask;

	return this->index;
}

bool CircularData::CircularIndex::operator==(const CircularIndex &tmp)
{
	assert(this->size == tmp.size);
	return (this->index == tmp.index);
}

bool CircularData::CircularIndex::operator!=(const CircularIndex &tmp)
{
	assert(this->size == tmp.size);
	return (this->index != tmp.index);
}

unsigned int CircularData::CircularIndex::Index() const
{
	return this->index;
}
//---  End of File ---

