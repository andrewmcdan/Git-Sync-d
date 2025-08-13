cmake_minimum_required(VERSION 3.10)
project(Git-Sync-d)

include(CTest)

set(CMAKE_CXX_STANDARD 17)

set(Boost_USE_STATIC_LIBS OFF) 
set(Boost_USE_MULTITHREADED ON)  
set(Boost_USE_STATIC_RUNTIME OFF) 

if(WIN32)
  set(BOOST_ROOT "C:/Users/Andrew/Documents/Projects/boost_1_84_0")
  set(BOOST_LIBRARYDIR "C:/Users/Andrew/Documents/Projects/boost_1_84_0/stage/lib/")
  find_package(Boost)
  if(NOT Boost_FOUND)
    set(BOOST_ROOT "C:/Users/Andrew/Documents/Projects/boost")
    set(BOOST_LIBRARYDIR "C:/Users/Andrew/Documents/Projects/boost/stage/lib/")
  endif()
elseif(APPLE)
  set(BOOST_ROOT "/Users/runner/boost")
  set(BOOST_LIBRARYDIR "/Users/runner/boost/stage/lib/")
elseif(UNIX AND NOT APPLE)
  set(BOOST_ROOT "/home/runner/boost")
  set(BOOST_LIBRARYDIR "/home/runner/boost/stage/lib/")
endif()

find_package(Boost)
if(Boost_FOUND)
  include_directories(${Boost_INCLUDE_DIRS})
  link_directories(${Boost_LIBRARY_DIRS})
  if(WIN32)
    add_definitions(-D_WIN32_WINNT=0x0601)
  endif()
endif()

# Platform-specific settings
if(WIN32)
  add_definitions(-DWINDOWS)
#   add_definitions(-D_WIN32_WINNT=0x0601)
  if(MSVC)
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd")
  endif()
elseif(UNIX AND NOT APPLE)
  add_definitions(-DLINUX)
elseif(APPLE)
  add_definitions(-DMAC)
endif()

# Include directories
include_directories(src/include/)

# Source files
file(GLOB SOURCES "src/*.cpp" "src/common/*.cpp")

# Platform-specific sources
if(WIN32)
  list(APPEND SOURCES "src/windows/service.cpp")
  list(APPEND SOURCES "src/windows/eventlog.cpp")
elseif(UNIX AND NOT APPLE)
  list(APPEND SOURCES "src/linux/daemon.cpp")
elseif(APPLE)
  list(APPEND SOURCES "src/mac/daemon.cpp")
endif()

add_executable(Git-Sync-d main.cpp ${SOURCES})

# Link against SQLite3
find_package(SQLite3 REQUIRED)
if(SQLite3_FOUND)
  include_directories(${SQLite3_INCLUDE_DIRS})
  target_link_libraries(Git-Sync-d PRIVATE SQLite::SQLite3)
endif()

find_library(CRYPTOPP_LIBRARY cryptopp)
if(CRYPTOPP_LIBRARY)
  target_link_libraries(Git-Sync-d PRIVATE ${CRYPTOPP_LIBRARY})
endif()


if(WIN32)
  find_program(MC_EXE mc.exe PATHS [ 
  "C:/Program Files (x86)/Windows Kits/10/bin/10.0.19041.0/x64/" 
  "C:/Program Files (x86)/Windows Kits/10/bin/" 
  ] REQUIRED)
  find_program(RC_EXE rc.exe PATHS [ 
  "C:/Program Files (x86)/Windows Kits/10/bin/10.0.19041.0/x64/" 
  "C:/Program Files (x86)/Windows Kits/10/bin/" 
  ] REQUIRED)
  find_program(LINK_EXE link.exe HINTS [ 
  "C:/Program Files (x86)/Microsoft Visual Studio/2019/Community/VC/" 
  "C:/Program Files/Microsoft Visual Studio/2022/Enterprise/VC/Tools/MSVC/" 
  "C:/Program Files/Microsoft Visual Studio/2022/"
  ] REQUIRED NO_CACHE)

  if(LINK_EXE MATCHES "C:\\Program Files\\Git\\usr\\bin\\link.exe" OR LINK_EXE MATCHES "\"C:\\Program Files\\Git\\usr\\bin\\link.exe\"")
    unset(LINK_EXE)
  endif()
  if(LINK_EXE MATCHES "C:/Program Files/Git/usr/bin/link.exe" OR LINK_EXE MATCHES "\"C:/Program Files/Git/usr/bin/link.exe\"")
    unset(LINK_EXE)
  endif()
  
  add_custom_target(Windows-Event-mc-build)

  add_custom_command(
    TARGET Windows-Event-mc-build
    COMMAND ${MC_EXE} -U ./GitSyncD.mc
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/src/windows/Event Log Resource Files/"
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/windows/Event Log Resource Files/GitSyncD.mc"
  )
  add_custom_command(
    TARGET Windows-Event-mc-build
    COMMAND ${RC_EXE} ./GitSyncD.rc
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/src/windows/Event Log Resource Files/"
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/windows/Event Log Resource Files/GitSyncD.mc"
  )
  if(LINK_EXE)
    add_custom_command(
      TARGET Windows-Event-mc-build
      COMMAND ${LINK_EXE} -dll -noentry ./GitSyncD.res -out:../../../build/bin/GitSyncD.dll
      WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/src/windows/Event Log Resource Files/"
      DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/windows/Event Log Resource Files/GitSyncD.mc"
    )
  else(
    add_custom_command(
      TARGET Windows-Event-mc-build
      COMMAND echo "Linker not found. Please install Visual Studio 2019 or 2022." > GitSyncd.dll.txt
      WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/build/bin/"
      DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/windows/Event Log Resource Files/GitSyncd.mc"
    )
  )
  endif()
  add_dependencies(Git-Sync-d Windows-Event-mc-build)
ENDIF()


if(BUILD_TESTING)
  include(FetchContent)
  FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.4.0
  )
  FetchContent_MakeAvailable(Catch2)
  file(GLOB TEST_SOURCES "tests/*.cpp")
  add_executable(GitSyncTests ${TEST_SOURCES} src/common/git.cpp src/common/ipc.cpp src/common/db.cpp src/common/error.cpp)
  target_compile_definitions(GitSyncTests PRIVATE UNIT_TESTING)
  target_include_directories(GitSyncTests PRIVATE src/common)
  if(Boost_FOUND)
    target_link_libraries(GitSyncTests PRIVATE ${Boost_LIBRARIES})
  endif()
  target_link_libraries(GitSyncTests PRIVATE Catch2::Catch2WithMain SQLite::SQLite3)
  add_test(NAME GitSyncTests COMMAND GitSyncTests)
endif()
