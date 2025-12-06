cmake_minimum_required(VERSION 3.10)
project(MazeSolver)

# 1. Default to Release mode
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Collect all source files
set(SOURCES
    CircularData.cpp
    Direction.cpp
    main.cpp
    maze.cpp
)

# 2. Setup Threading Library
find_package(Threads REQUIRED)

# Add executable
add_executable(MazeSolver ${SOURCES})

# Include directories
target_include_directories(MazeSolver PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# Link threading library
target_link_libraries(MazeSolver PRIVATE Threads::Threads)

# -----------------------------------------------------------------------------
# 3. INSTANT DATA COPYING (Configuration Time)
# -----------------------------------------------------------------------------
# Define the source directory for data
set(MAZE_DATA_SRC "${CMAKE_CURRENT_SOURCE_DIR}/Maze_Data")

# 1. Get a list of all files inside the maze_data folder
file(GLOB DATA_FILES "${MAZE_DATA_SRC}/*")

# 2. Check if we actually found files (optional warning)
list(LENGTH DATA_FILES NUM_FILES)
if(NUM_FILES EQUAL 0)
    message(WARNING "No files found in ${MAZE_DATA_SRC}. The solver might fail at runtime.")
else()
    message(STATUS "Found ${NUM_FILES} maze data files. Copying to build directory...")
endif()

# 3. Copy them immediately to the build folder
# This happens right now, when you run 'cmake'
file(COPY ${DATA_FILES} DESTINATION "${CMAKE_BINARY_DIR}")

# -----------------------------------------------------------------------------
# 4. OPTIMIZATIONS
# -----------------------------------------------------------------------------

# Check for Interprocedural Optimization (LTO/IPO).
include(CheckIPOSupported)
check_ipo_supported(RESULT result OUTPUT output)
if(result)
    set_property(TARGET MazeSolver PROPERTY INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE)
else()
    message(WARNING "IPO is not supported: ${output}")
endif()

# Compiler specific optimization flags for GCC/Clang (Linux Standard)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(MazeSolver PRIVATE
        $<$<CONFIG:Release>:-O2>             # Maximum optimization level
        $<$<CONFIG:Release>:-march=native>   # KEY for Atomics: Use instructions specific to YOUR CPU
        $<$<CONFIG:Release>:-mtune=native>   # Tune scheduling for your CPU
        $<$<CONFIG:Release>:-funroll-loops>  # Aggressive loop unrolling
        $<$<CONFIG:Release>:-Wall>           # Good practice to see warnings
        $<$<CONFIG:Release>:-pthread>        # Ensure thread model is set
    )
endif()

# Visual Studio (MSVC) flags
if(MSVC)
    target_compile_options(MazeSolver PRIVATE
        $<$<CONFIG:Release>:/Ox>    # Max optimization
        $<$<CONFIG:Release>:/Ob2>   # Aggressive inline
        $<$<CONFIG:Release>:/Oi>    # Intrinsic functions
    )
endif()
