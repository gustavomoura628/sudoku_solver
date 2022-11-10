# Sudoku Solver

A sudoku solver for arbitrary sized boards that uses constraint propagation, dfs and set representation using bits to find a solution.

## Set up
gcc -o sudoku sudoku.c

## Usage
./sudoku

obs: blocksize is the size of a block, for example: in a 9x9 board blocks are 3x3, thus their size is 3.
