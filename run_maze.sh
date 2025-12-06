#!/bin/bash

echo "Running MazeSolver"

files=(
    "Maze1Kx1K.data"
    "Maze2Kx2K.data"
    "Maze5Kx5K.data"
    "Maze10Kx10K.data"
    "Maze15Kx15K.data"
    "Maze15Kx15K_E.data"
    "Maze15Kx15K_J.data"
    "Maze20Kx20K_B.data"
    "Maze20Kx20K_D.data"
)

for f in "${files[@]}"
do
    if [[ -f "$f" ]]; then
        echo "Running MazeSolver on $f ..."
        ./MazeSolver "$f"
    else
        echo "File not found: $f (skipping)"
    fi
done

echo "Finished!"
