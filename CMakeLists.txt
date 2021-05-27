# Top-Level CMakeList.txt

cmake_minimum_required (VERSION 3.9)
project ( Project )

set(CMAKE_CXX_STANDARD 20)

find_package(OpenMP REQUIRED)
find_package(SDL REQUIRED)
find_package(glm REQUIRED)
find_package(MPI)

add_executable( Project main.cpp)

if ( NOT SDL_FOUND )
   message ( FATAL_ERROR "SDL not found!" )
endif ( NOT SDL_FOUND )

if(SDL_FOUND)
	include_directories(
		${SDL_INCLUDE_DIR}
	)
	target_link_libraries(Project PRIVATE ${SDL_LIBRARY})
endif(SDL_FOUND)

target_link_libraries(Project PRIVATE glm::glm OpenMP::OpenMP_CXX)

if (MPI_CXX_FOUND)
	target_link_libraries(Project PRIVATE MPI::MPI_CXX)
endif(MPI_CXX_FOUND)

