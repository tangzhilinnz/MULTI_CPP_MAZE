cmake_minimum_required(VERSION 3.10)
project(MazeSolver)

# 1. Default to Release mode if the user didn't specify one.
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
# 3. AUTOMATIC DATA COPYING (FROM maze_data FOLDER)
# -----------------------------------------------------------------------------
# This looks for the 'maze_data' folder in your project root
set(MAZE_DATA_DIR "${CMAKE_CURRENT_SOURCE_DIR}/maze_data")

# Scans for ALL files inside maze_data
file(GLOB DATA_FILES "${MAZE_DATA_DIR}/*")

# Adds a post-build step to copy these files to the executable's folder
foreach(DATA_FILE ${DATA_FILES})
    add_custom_command(TARGET MazeSolver POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${DATA_FILE}"
            "$<TARGET_FILE_DIR:MazeSolver>"
        COMMENT "Copying data file from maze_data/ to build directory..."
    )
endforeach()

# -----------------------------------------------------------------------------
# 4. OPTIMIZATIONS
# -----------------------------------------------------------------------------

# Check if the compiler supports Interprocedural Optimization (LTO/IPO).
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
        $<$<CONFIG:Release>:-O3>             # Maximum optimization level
        $<$<CONFIG:Release>:-march=native>   # KEY for Atomics: Use instructions specific to YOUR CPU
        $<$<CONFIG:Release>:-mtune=native>   # Tune scheduling for your CPU
        $<$<CONFIG:Release>:-funroll-loops>  # Aggressive loop unrolling
        $<$<CONFIG:Release>:-Wall>           # Good practice to see warnings
        $<$<CONFIG:Release>:-pthread>        # Ensure thread model is set
    )
endif()

# Visual Studio (MSVC) flags if you ever run this back on Windows
if(MSVC)
    target_compile_options(MazeSolver PRIVATE
        $<$<CONFIG:Release>:/Ox>    # Max optimization
        $<$<CONFIG:Release>:/Ob2>   # Aggressive inline
        $<$<CONFIG:Release>:/Oi>    # Intrinsic functions
    )
endif()
