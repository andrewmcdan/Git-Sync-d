cmake_minimum_required(VERSION 3.10)
project(Git-Sync-d)

set(CMAKE_CXX_STANDARD 17)

set(Boost_USE_STATIC_LIBS OFF) 
set(Boost_USE_MULTITHREADED ON)  
set(Boost_USE_STATIC_RUNTIME OFF) 

set(Boost_INCLUDE_DIR "C:/Users/Andrew/Documents/Projects/boost_1_84_0")
set(Boost_LIBRARY_DIR "C:/Users/Andrew/Documents/Projects/boost_1_84_0/stage/lib/")

find_package(Boost)
if(Boost_FOUND)
  include_directories(${Boost_INCLUDE_DIRS})
  link_directories(${Boost_LIBRARY_DIRS})
endif()

# Platform-specific settings
if(WIN32)
  add_definitions(-DWINDOWS)
  if(MSVC)
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd")
  endif()
elseif(UNIX AND NOT APPLE)
  add_definitions(-DLINUX)
endif()

# Include directories
include_directories(src/include/)

# Source files
file(GLOB SOURCES "src/*.cpp" "src/common/*.cpp")

# Platform-specific sources
if(WIN32)
  list(APPEND SOURCES "src/windows/service.cpp")
elseif(UNIX AND NOT APPLE)
  list(APPEND SOURCES "src/linux/daemon.cpp")
endif()

add_executable(Git-Sync-d main.cpp ${SOURCES})