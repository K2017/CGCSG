# Top-Level CMakeList.txt

cmake_minimum_required (VERSION 3.9)
project ( SDFCSG )

set(CMAKE_CXX_STANDARD 20)

find_package(OpenMP)
find_package(SDL REQUIRED)
find_package(glm REQUIRED)

add_executable( SDFCSG main.cpp)

if ( NOT SDL_FOUND )
   message ( FATAL_ERROR "SDL not found!" )
endif ( NOT SDL_FOUND )

if(SDL_FOUND)
	include_directories(
		${SDL_INCLUDE_DIR}
	)
	target_link_libraries(SDFCSG PRIVATE ${SDL_LIBRARY})
endif(SDL_FOUND)

if (OpenMP_CXX_FOUND)
	target_link_libraries(SDFCSG PRIVATE OpenMP::OpenMP_CXX)
endif(OpenMP_CXX_FOUND)

target_link_libraries(SDFCSG PRIVATE glm::glm)