//-----------------------------------------------------------------------------
// Copyright 2025, Ed Keenan, all rights reserved.
//----------------------------------------------------------------------------- 

#include "CircularData.h"

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
	//// initialized data
	//for(int i = 0; i < CIRCULAR_DATA_SIZE; i++)
	//{
	//	this->data[i] = 0;
	//}
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
			Trace::out("full ===================\n");
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



//#include "CircularData.h"
//#include <cassert>
//
//CircularData::CircularData()
//{
//    head.store(0, std::memory_order_relaxed);
//    tail.store(0, std::memory_order_relaxed);
//}
//
//// -------------------------------------------------------------
//// PushBack (Producer only) - const Position&
//// -------------------------------------------------------------
//bool CircularData::PushBack(const Position& pPos)
//{
//    size_t t = tail.load(std::memory_order_relaxed);
//    size_t h = head.load(std::memory_order_acquire);
//
//    size_t next = (t + 1) & (CIRCULAR_DATA_SIZE - 1);
//
//    if (next == h)
//    {
//        // full
//        Trace::out("full ===================\n");
//
//        return false;
//    }
//
//    buffer[t] = pPos;
//
//    // publish write to consumer
//    tail.store(next, std::memory_order_release);
//
//    return true;
//}
//
//// -------------------------------------------------------------
//// PushBack (Producer only) - rvalue
//// -------------------------------------------------------------
//bool CircularData::PushBack(Position&& pPos)
//{
//    size_t t = tail.load(std::memory_order_relaxed);
//    size_t h = head.load(std::memory_order_acquire);
//
//    size_t next = (t + 1) & (CIRCULAR_DATA_SIZE - 1);
//
//    if (next == h)
//    {
//
//        Trace::out("full ===================\n");
//        return false; // full
//    }
//
//    buffer[t] = std::move(pPos);
//
//    tail.store(next, std::memory_order_release);
//
//    return true;
//}
//
//// -------------------------------------------------------------
//// PopFront (Consumer only)
//// -------------------------------------------------------------
//bool CircularData::PopFront(Position& pPos)
//{
//    size_t h = head.load(std::memory_order_relaxed);
//    size_t t = tail.load(std::memory_order_acquire);
//
//    if (h == t)
//    {
//        // empty
//        return false;
//    }
//
//    pPos = buffer[h];
//
//    head.store((h + 1) & (CIRCULAR_DATA_SIZE - 1),
//        std::memory_order_release);
//
//    return true;
//}
//
//// -------------------------------------------------------------
//// Check empty (safe for SPSC)
//// -------------------------------------------------------------
//bool CircularData::IsEmpty() const
//{
//    return head.load(std::memory_order_acquire) ==
//        tail.load(std::memory_order_acquire);
//}
//
//// -------------------------------------------------------------
//// Check full (safe for SPSC)
//// -------------------------------------------------------------
//bool CircularData::IsFull() const
//{
//    size_t h = head.load(std::memory_order_acquire);
//    size_t t = tail.load(std::memory_order_acquire);
//
//    return ((t + 1) & (CIRCULAR_DATA_SIZE - 1)) == h;
//}

