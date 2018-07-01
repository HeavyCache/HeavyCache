cmake_minimum_required (VERSION 3.2)
project(HeavyCacheDemo)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pedantic -Wall -Wextra -Wredundant-decls")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -msse2 -mssse3 -msse4.1 -msse4.2 -mavx -mavx2 -march=native")
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ggdb3")


ADD_DEFINITIONS(-DPROJECT_ROOT="${CMAKE_SOURCE_DIR}")
SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin")

ADD_EXECUTABLE(main src/main.cpp)
