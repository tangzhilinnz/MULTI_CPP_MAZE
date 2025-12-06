#ifndef DIRECTION_MAZE_H
#define DIRECTION_MAZE_H

enum class Direction
{
	North, 
	East, 
	South, 
	West,
	Uninitialized,
};

Direction reverseDir( Direction dir );

class DirectionHelper
{
public:
	DirectionHelper() = default;
	DirectionHelper(const DirectionHelper &) = delete;
	DirectionHelper &operator=(const DirectionHelper &) = delete;
	~DirectionHelper() = default;

	static const char *getString( Direction dir );
};

#endif

// --- End of File ----
