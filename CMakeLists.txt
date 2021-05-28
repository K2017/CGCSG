# Top-Level CMakeList.txt

cmake_minimum_required (VERSION 3.9)
project ( Project )

set(CMAKE_CXX_STANDARD 20)

find_package(OpenMP REQUIRED)
find_package(SDL REQUIRED)
find_package(glm REQUIRED)

add_executable( Project main.cpp)

if ( NOT SDL_FOUND )
   message ( FATAL_ERROR "SDL not found!" )
endif ( NOT SDL_FOUND )

if(SDL_FOUND)
	include_directories(
		${SDL_INCLUDE_DIR}
		#${PROJECT_SOURCE_DIR}/glm
	)
	#link_libraries(${SDL_LIBRARY})
	target_link_libraries(Project PRIVATE ${SDL_LIBRARY})
endif(SDL_FOUND)

target_link_libraries(Project PRIVATE glm::glm OpenMP::OpenMP_CXX)