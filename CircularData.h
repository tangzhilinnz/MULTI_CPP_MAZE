#ifndef CIRCULAR_DATA_H
#define CIRCULAR_DATA_H

#include "Position.h"
#include <mutex>

class CircularData
{
private:
	class CircularIndex
	{
	public:
		CircularIndex(unsigned int buffSize);

		CircularIndex() = delete;
		CircularIndex(const CircularIndex &) = delete;
		const CircularIndex &operator = (const CircularIndex &) = delete;
		~CircularIndex() = default;

		// postfix
		unsigned int operator++(int);
		bool operator==(const CircularIndex &tmp);
		bool operator!=(const CircularIndex &tmp);

		// accessor
		unsigned int Index() const;

	private:
		unsigned int index;
		unsigned int size;
		unsigned int mask;
	};

public:
	// Needs to be a power of 2
	static const int CIRCULAR_DATA_SIZE = 1 << 13;

	// verify that buffSize is a power of 2
	static_assert((CIRCULAR_DATA_SIZE &(CIRCULAR_DATA_SIZE - 1)) == 0, "not Power of 2");

public:
	CircularData();

	CircularData(const CircularData &) = delete;
	const CircularData &operator = (const CircularData &) = delete;
	~CircularData() = default;

	bool PushBack(Position pPos);
	bool PopFront(Position& pPos);

	bool IsEmpty();

private:
	Position data[CIRCULAR_DATA_SIZE];

	CircularIndex front;
	CircularIndex back;
	bool empty;
	bool full;
	char pad[6];
	std::mutex mtx;
};

#endif

//---  End of File ---
