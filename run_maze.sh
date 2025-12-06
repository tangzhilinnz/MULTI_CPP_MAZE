#!/bin/bash

echo "Running MazeSolver for all .data files..."

for file in *.data
do
    echo "Running MazeSolver on $file ..."
        ./MazeSolver "$file"
        done

echo "Finished!"
