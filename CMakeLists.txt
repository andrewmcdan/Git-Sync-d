cmake_minimum_required(VERSION 3.10)
project(Git-Sync-d)

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
include_directories(include/)

# Source files
file(GLOB SOURCES "src/*.cpp" "src/common/*.cpp")

# Platform-specific sources
if(WIN32)
  list(APPEND SOURCES "src/windows/service.cpp")
elseif(UNIX AND NOT APPLE)
  list(APPEND SOURCES "src/linux/daemon.cpp")
endif()

add_executable(Git-Sync-d main.cpp ${SOURCES})