#include <cstdio>
#include <cstring>
#include <vector>
#include <chrono>

#include "STMazeSolverDFS.h"
#include "STMazeSolverBFS.h"
#include "MTMazeStudentSolver.h"

#define INPUT_NAME_SIZE 64

// change to "1" for the final submission
// do not run in debugger... copy exe to data directory
#define FINAL_SUBMIT 1

// --- Linux Compatible Timer (High Precision) ---
// Uses high_resolution_clock to ensure the most accurate 
// fractional seconds (decimal point) possible on the system.
class PerformanceTimer
{
public:
	void Tic()
	{
		start_time = std::chrono::high_resolution_clock::now();
	}
	void Toc()
	{
		end_time = std::chrono::high_resolution_clock::now();
	}
	double TimeInSeconds()
	{
		// Casts duration to double-precision seconds
		std::chrono::duration<double> diff = end_time - start_time;
		return diff.count();
	}
private:
	std::chrono::time_point<std::chrono::high_resolution_clock> start_time;
	std::chrono::time_point<std::chrono::high_resolution_clock> end_time;
};


int main(int argc, char *argv[]) 
{
	(void)argc;

	printf("--MAIN--\n");

#if !FINAL_SUBMIT
	char inFileName[INPUT_NAME_SIZE] = "Maze20Kx20K_B.data";
	(void)argv;
#else
	char inFileName[INPUT_NAME_SIZE];
	memset(inFileName,0x0,INPUT_NAME_SIZE);
	if (argc > 1)
	{
		snprintf(inFileName, INPUT_NAME_SIZE, "%s", argv[1]);
	}
	else
	{
		printf("Error: FINAL_SUBMIT is 1, but no filename argument provided.\n");
		return 1;
	}
#endif

	printf("\n");
	printf("Maze: start(%s) ------------\n",inFileName);

	// Create timers 
	PerformanceTimer ST_DFS_Timer;
	PerformanceTimer ST_BFS_Timer;
	PerformanceTimer MT_Student_Timer_1;
	PerformanceTimer MT_Student_Timer_2;
	 
	// Create a maze
	Maze *pMaze;

	// -- STMazeSolverBFS -----------------------------------------------------------
	printf("\n Maze: STMazeSolverBFS\n");

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
	printf("\n Maze: STMazeSolverDFS\n");

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
	printf("\n Maze: MTStudentSolver_1\n");

		// Create and load Maze - Data is atomic_int
		pMaze = new Maze();
		pMaze->Load(inFileName);

		// Solve it
		MT_Student_Timer_1.Tic(); 

			MTMazeStudentSolver mtStudentSolverA(pMaze);
			std::vector<Direction> *pSolutionStudentA = mtStudentSolverA.Solve_1();
		
		MT_Student_Timer_1.Toc();

		// Verify solution - do not delete, it is not timed
		pMaze->checkSolution( *pSolutionStudentA );

		// release memory
		delete pSolutionStudentA;
		delete pMaze;

	printf("\n Maze: MTStudentSolver_2\n");

		// Create and load Maze - Data is atomic_int
		pMaze = new Maze();
		pMaze->Load(inFileName);

		// Solve it
		MT_Student_Timer_2.Tic();

		MTMazeStudentSolver mtStudentSolverB(pMaze);
		std::vector<Direction>* pSolutionStudentB = mtStudentSolverB.Solve_2();

		MT_Student_Timer_2.Toc();

		// Verify solution - do not delete, it is not timed
		pMaze->checkSolution(*pSolutionStudentB);

		// release memory
		delete pSolutionStudentB;
		delete pMaze;

	// Stats --------------------------------------------------------------------------

		double ST_DFSTime = ST_DFS_Timer.TimeInSeconds();
		double ST_BFSTime = ST_BFS_Timer.TimeInSeconds();
		double MT_StudentTime_1 = MT_Student_Timer_1.TimeInSeconds();
		double MT_StudentTime_2 = MT_Student_Timer_2.TimeInSeconds();

		printf("\n");
		printf("Results (%s):\n\n",inFileName);
		printf("   BFS      : %f s\n",ST_BFSTime);
		printf("   DFS      : %f s\n",ST_DFSTime);
		printf("   MT_M1    : %f s\n", MT_StudentTime_1);
		printf("   MT_M2    : %f s\n", MT_StudentTime_2);

		printf("\nMaze: end() --------------\n\n");

}

// --- End of File ----



