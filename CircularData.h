//#ifndef CIRCULAR_DATA_H
//#define CIRCULAR_DATA_H
//
//#include "Position.h"
//#include <mutex>
//
//class CircularData
//{
//private:
//	class CircularIndex
//	{
//	public:
//		CircularIndex(unsigned int buffSize);
//
//		CircularIndex() = delete;
//		CircularIndex(const CircularIndex &) = delete;
//		const CircularIndex &operator = (const CircularIndex &) = delete;
//		~CircularIndex() = default;
//
//		// postfix
//		unsigned int operator++(int);
//		bool operator==(const CircularIndex &tmp);
//		bool operator!=(const CircularIndex &tmp);
//
//		// accessor
//		unsigned int Index() const;
//
//	private:
//		unsigned int index;
//		unsigned int size;
//		unsigned int mask;
//	};
//
//public:
//	// Needs to be a power of 2
//	static const int CIRCULAR_DATA_SIZE = 1 << 13;
//
//	// verify that buffSize is a power of 2
//	static_assert((CIRCULAR_DATA_SIZE &(CIRCULAR_DATA_SIZE - 1)) == 0, "not Power of 2");
//
//public:
//	CircularData();
//
//	CircularData(const CircularData &) = delete;
//	const CircularData &operator = (const CircularData &) = delete;
//	~CircularData() = default;
//
//	bool PushBack(Position pPos);
//	bool PopFront(Position& pPos);
//
//	bool IsEmpty();
//
//private:
//	Position data[CIRCULAR_DATA_SIZE];
//
//	CircularIndex front;
//	CircularIndex back;
//	bool empty;
//	bool full;
//	char pad[6];
//	std::mutex mtx;
//};
//
//#endif
//
////---  End of File ---


#ifndef CIRCULAR_DATA_H
#define CIRCULAR_DATA_H

#include "Position.h"
#include <mutex>
#include <cassert>

class CircularData
{
private:
	class CircularIndex
	{
	public:
		CircularIndex(unsigned int buffSize)
			: index(0), size(buffSize), mask(buffSize - 1)
		{
			assert((size & (size - 1)) == 0);
		}

		CircularIndex() = delete;
		CircularIndex(const CircularIndex&) = delete;
		const CircularIndex& operator=(const CircularIndex&) = delete;

		unsigned int operator++(int)
		{
			unsigned int old = index;
			index = (index + 1) & mask;
			return old;
		}

		unsigned int NextIndex() const
		{
			return (index + 1) & mask;
		}

		bool operator==(const CircularIndex& tmp) const
		{
			return index == tmp.index;
		}

		bool operator!=(const CircularIndex& tmp) const
		{
			return index != tmp.index;
		}

		unsigned int Index() const { return index; }

	private:
		unsigned int index;
		unsigned int size;
		unsigned int mask;
	};

public:
	static const unsigned int CIRCULAR_DATA_SIZE = (1 << 13);
	static_assert((CIRCULAR_DATA_SIZE& (CIRCULAR_DATA_SIZE - 1)) == 0,
		"Size must be power of 2");

public:
	CircularData();
	~CircularData() = default;

	CircularData(const CircularData&) = delete;
	const CircularData& operator=(const CircularData&) = delete;

	bool PushBack(const Position& pPos);
	bool PopFront(Position& pPos);
	bool IsEmpty() const;
	bool IsFull() const;
	size_t Size() const;

private:
	alignas(64) Position data[CIRCULAR_DATA_SIZE];
	char pad1[64 - (CIRCULAR_DATA_SIZE * sizeof(Position)) % 64];
	alignas(64) CircularIndex head;
	char pad2[64 - sizeof(CircularIndex)];
	alignas(64) CircularIndex tail;
	char pad3[64 - sizeof(CircularIndex)];
	alignas(64) mutable std::mutex mtx;
	char pad4[48];
};

#endif
