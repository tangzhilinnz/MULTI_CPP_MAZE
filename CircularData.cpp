//-----------------------------------------------------------------------------
// Copyright 2025, Ed Keenan, all rights reserved.
//----------------------------------------------------------------------------- 

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


//-----------------------------------------------------------------------------
// Copyright 2025, Ed Keenan, all rights reserved.
//----------------------------------------------------------------------------- 

#include "CircularData.h"

CircularData::CircularData()
    : head(0),
    tail(0)
{
}

bool CircularData::IsEmpty() const
{
    // If head and tail match, there is no data
    // using Relaxed because this is usually just a state check, 
    // strictly speaking, Acquire is safer if you act on it immediately externally.
    return (this->head.load(std::memory_order_relaxed) == this->tail.load(std::memory_order_relaxed));
}

bool CircularData::PushBack(Position pPos)
{
    // 1. Load the current Write Index (Relaxed is fine, only WE change it)
    const auto currentHead = this->head.load(std::memory_order_relaxed);

    // 2. Calculate next Write Index
    const auto nextHead = (currentHead + 1) & MASK;

    // 3. Load the Read Index (Acquire)
    // We need Acquire here to ensure that any reads of the slot we are about to 
    // write to (by the consumer) are finished before we overwrite it.
    const auto currentTail = this->tail.load(std::memory_order_acquire);

    // 4. Check if Full (One Slot Empty Rule)
    if (nextHead == currentTail)
    {
        // Queue is full
        return false;
    }

    // 5. Write Data
    // We are safe to write because we know the consumer isn't reading this slot (Step 4)
    this->data[currentHead] = pPos;

    // 6. Update Write Index (Release)
    // We use Release to ensure the write to 'data' (Step 5) is visible to the 
    // consumer BEFORE they see the new head index.
    this->head.store(nextHead, std::memory_order_release);

    return true;
}

bool CircularData::PopFront(Position& pPos)
{
    // 1. Load the current Read Index (Relaxed is fine, only WE change it)
    const auto currentTail = this->tail.load(std::memory_order_relaxed);

    // 2. Load the Write Index (Acquire)
    // We need Acquire here to ensure that the data written by the producer 
    // is fully visible to us before we try to read it.
    const auto currentHead = this->head.load(std::memory_order_acquire);

    // 3. Check if Empty
    if (currentTail == currentHead)
    {
        // Queue is empty
        return false;
    }

    // 4. Read Data
    pPos = this->data[currentTail];

    // 5. Calculate next Read Index
    const auto nextTail = (currentTail + 1) & MASK;

    // 6. Update Read Index (Release)
    // We use Release so the producer knows we are done with this slot 
    // and they can safely overwrite it in the future.
    this->tail.store(nextTail, std::memory_order_release);

    return true;
}

//---  End of File ---

