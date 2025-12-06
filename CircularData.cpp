////-----------------------------------------------------------------------------
//// Copyright 2025, Ed Keenan, all rights reserved.
////----------------------------------------------------------------------------- 
//
//#include "CircularData.h"
//#include <assert.h>
//#include <cstdio>
//#include <cstring>
//
//bool CircularData::IsEmpty()
//{
//	return this->empty;
//}
//
//CircularData::CircularData()
//	: front(CIRCULAR_DATA_SIZE),
//	back(CIRCULAR_DATA_SIZE),
//	empty(true),
//	full(false)
//{
//}
//
//bool CircularData::PushBack(Position pPos)
//{
//	std::lock_guard<std::mutex> lock(this->mtx);
//
//	bool status = true;
//
//	// Do we have space to add?
//	if(this->front != this->back || this->empty)
//	{
//		// add data
//		this->data[this->back.Index()] = pPos;
//		this->back++;
//
//		this->empty = false;
//
//		// check if this add made it full
//		if(this->front == this->back)
//		{
//			this->full = true;
//			printf("full ===================\n");
//			// Safety
//			assert(false);
//		}
//	}
//	else
//	{
//		status = false;
//	}
//
//	return status;
//}
//
//bool CircularData::PopFront(Position& pPos)
//{
//	std::lock_guard<std::mutex> lock(this->mtx);
//
//	bool status = true;
//
//	// Is there data to process?
//	if(this->front != this->back || this->full)
//	{
//		// Grab one
//		pPos = this->data[this->front.Index()];
//		this->front++;
//
//		this->full = false;
//
//		// check if this Pop made it Empty
//		if(this->front == this->back)
//		{
//			this->empty = true;
//		}
//	}
//	else
//	{
//		status = false;
//	}
//	return status;
//}
//
//
//CircularData::CircularIndex::CircularIndex(unsigned int buffSize)
//{
//
//	this->size = buffSize;
//	this->mask = (unsigned int)(buffSize - 1);
//
//	// verify that buffSize is a power of 2
//	assert((size & (size - 1)) == 0);
//
//	this->index = 0;
//}
//
//// postfix
//unsigned int CircularData::CircularIndex::operator++(int)
//{
//	this->index++;
//
//	// Circular 
//	this->index = this->index & this->mask;
//
//	return this->index;
//}
//
//bool CircularData::CircularIndex::operator==(const CircularIndex &tmp)
//{
//	assert(this->size == tmp.size);
//	return (this->index == tmp.index);
//}
//
//bool CircularData::CircularIndex::operator!=(const CircularIndex &tmp)
//{
//	assert(this->size == tmp.size);
//	return (this->index != tmp.index);
//}
//
//unsigned int CircularData::CircularIndex::Index() const
//{
//	return this->index;
//}
////---  End of File ---
//

//-----------------------------------------------------------------------------
// Copyright 2025, Ed Keenan, all rights reserved.
//----------------------------------------------------------------------------- 

#include "CircularData.h"

CircularData::CircularData()
	: front(0),
	back(0)
{
}

bool CircularData::IsEmpty()
{
	// Acquire ensures we see the latest writes from the other thread
	return this->front.load(std::memory_order_acquire) == this->back.load(std::memory_order_relaxed);
}

bool CircularData::PushBack(Position pPos)
{
	// 1. Load the indices
	// We use Relaxed for 'back' because this thread OWNS it (Producer).
	// We use Acquire for 'front' to ensure we see the Consumer's latest updates.
	unsigned int currentBack = this->back.load(std::memory_order_relaxed);
	unsigned int currentFront = this->front.load(std::memory_order_acquire);

	// 2. Calculate next position
	unsigned int nextBack = (currentBack + 1) & MASK;

	// 3. Check if full
	// If nextBack catches up to Front, we are full.
	if (nextBack == currentFront)
	{
		return false; // Queue is full
	}

	// 4. Write Data
	// Crucial: We write data BEFORE updating the index.
	this->data[currentBack] = pPos;

	// 5. Publish the write
	// Release ensures the consumer sees the data written above 
	// when they see this new index value.
	this->back.store(nextBack, std::memory_order_release);

	return true;
}

bool CircularData::PopFront(Position& pPos)
{
	// 1. Load the indices
	// We use Relaxed for 'front' because this thread OWNS it (Consumer).
	// We use Acquire for 'back' to ensure we see the Producer's latest updates.
	unsigned int currentFront = this->front.load(std::memory_order_relaxed);
	unsigned int currentBack = this->back.load(std::memory_order_acquire);

	// 2. Check if Empty
	if (currentFront == currentBack)
	{
		return false; // Queue is empty
	}

	// 3. Read Data
	// Safe to read because Producer promised data is ready via 'release' on 'back'.
	pPos = this->data[currentFront];

	// 4. Calculate next position
	unsigned int nextFront = (currentFront + 1) & MASK;

	// 5. Publish the read
	// Release tells the producer "I am done with this slot, you can reuse it".
	this->front.store(nextFront, std::memory_order_release);

	return true;
}
