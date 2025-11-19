//------------------------------------------------------
// Copyright 2025, Ed Keenan, all rights reserved.
//------------------------------------------------------

#include "STMazeSolverDFS.h"
#include "STMazeSolverBFS.h"
#include "MTMazeStudentSolver.h"

#define INPUT_NAME_SIZE 64

// change to "1" for the final submission
// do not run in debugger... copy exe to data directory
#define FINAL_SUBMIT 0


int main(int argc, char *argv[]) 
{
	AZUL_UNUSED_VAR(argc);
	AZUL_UNUSED_VAR(argv);

	START_BANNER_MAIN("--MAIN--");

#if !FINAL_SUBMIT
	char inFileName[INPUT_NAME_SIZE] = "Maze20Kx20K_B.data";
#else
	char inFileName[INPUT_NAME_SIZE];
	memset(inFileName,0x0,INPUT_NAME_SIZE);
	strcpy_s( inFileName, INPUT_NAME_SIZE, argv[1]);
#endif

	Trace::out("\n");
	Trace::out2("Maze: start(%s) ------------\n",inFileName);

	// Create timers 
	PerformanceTimer ST_DFS_Timer;
	PerformanceTimer ST_BFS_Timer;
	PerformanceTimer MT_Student_Timer;
	 
	// Create a maze
	Maze *pMaze;

	// -- STMazeSolverBFS -----------------------------------------------------------
	Trace::out2("\n Maze: STMazeSolverBFS\n");

	// Create and load Maze - Data is atomic_int
		pMaze = new Maze();
		pMaze->Load(inFileName);

		// Solve it
		ST_BFS_Timer.Tic(); 

			STMazeSolverBFS stSolverBFS(pMaze);
			std::vector<Direction> *pSolutionBFS = stSolverBFS.Solve();
		
		ST_BFS_Timer.Toc();

		// Verify solution - do not delete, it is not timed
		pMaze->checkSolution( *pSolutionBFS );

		// release memory
		delete pSolutionBFS;
		delete pMaze;

	// -- STMazeSolverDFS -----------------------------------------------------------
	Trace::out2("\n Maze: STMazeSolverDFS\n");

		// Create and load Maze - Data is atomic_int
		pMaze = new Maze();
		pMaze->Load(inFileName);

		// Solve it
		ST_DFS_Timer.Tic(); 

			STMazeSolverDFS stSolverDFS(pMaze);
			std::vector<Direction> *pSolutionDFS = stSolverDFS.Solve();
		
		ST_DFS_Timer.Toc();

		// Verify solution - do not delete, it is not timed
		pMaze->checkSolution( *pSolutionDFS );

		// release memory
		delete pSolutionDFS;
		delete pMaze;

	// -- Multi-Threaded Student Solver ----------------------------------------------
	Trace::out2("\n Maze: MTStudentSolver\n");

		// Create and load Maze - Data is atomic_int
		pMaze = new Maze();
		pMaze->Load(inFileName);

		// Solve it
		MT_Student_Timer.Tic(); 

			MTMazeStudentSolver mtStudentSolver(pMaze);
			std::vector<Direction> *pSolutionStudent = mtStudentSolver.Solve();
		
		MT_Student_Timer.Toc();

		// Verify solution - do not delete, it is not timed
		pMaze->checkSolution( *pSolutionStudent );

		// release memory
		delete pSolutionStudent;
		delete pMaze;

	// Stats --------------------------------------------------------------------------

		double ST_DFSTime = ST_DFS_Timer.TimeInSeconds();
		double ST_BFSTime = ST_BFS_Timer.TimeInSeconds();
		double MT_StudentTime = MT_Student_Timer.TimeInSeconds();

		Trace::out2("\n");
		Trace::out2("Results (%s):\n\n",inFileName);
		Trace::out2("   BFS      : %f s\n",ST_BFSTime);
		Trace::out2("   DFS      : %f s\n",ST_DFSTime);
		Trace::out2("   Student  : %f s\n",MT_StudentTime);

	Trace::out2("\nMaze: end() --------------\n\n"); 

}

// --- End of File ----

