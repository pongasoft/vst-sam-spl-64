cmake_minimum_required(VERSION 3.12)

include(FetchContent)

set(R8BRAIN-FREE-SRC_GIT_REPO "https://github.com/avaneev/r8brain-free-src" CACHE STRING "r8brain-free-src git repository url" FORCE)
set(R8BRAIN-FREE-SRC_GIT_TAG 6453d7756c1449afda66cc98a9b406006900fc13 CACHE STRING "r8brain-free-src git tag" FORCE)

if(R8BRAIN-FREE-SRC_ROOT_DIR)
  message(STATUS "Using r8brain-free-src from local ${R8BRAIN-FREE-SRC_ROOT_DIR}")
  FetchContent_Populate(r8brain-free-src
      QUIET
      SOURCE_DIR        "${R8BRAIN-FREE-SRC_ROOT_DIR}"
      BINARY_DIR        "${CMAKE_BINARY_DIR}/r8brain-free-src-build"
  )
else()
  message(STATUS "Fetching r8brain-free-src ${R8BRAIN-FREE-SRC_GIT_REPO}/tree/${R8BRAIN-FREE-SRC_GIT_TAG}")
  FetchContent_Populate(r8brain-free-src
      GIT_REPOSITORY    ${R8BRAIN-FREE-SRC_GIT_REPO}
      GIT_TAG           ${R8BRAIN-FREE-SRC_GIT_TAG}
      GIT_CONFIG        advice.detachedHead=false
      GIT_SHALLOW       false
      SOURCE_DIR        "${CMAKE_BINARY_DIR}/r8brain-free-src"
      BINARY_DIR        "${CMAKE_BINARY_DIR}/r8brain-free-src-build"
  )
endif()


# No CMakeLists.txt included => creating one
file(WRITE "${r8brain-free-src_SOURCE_DIR}/CMakeLists.txt"
           [=[
project(r8brain-free-src)
add_library(r8brain-free-src r8bbase.cpp)
]=]
    )

add_subdirectory(${r8brain-free-src_SOURCE_DIR} ${r8brain-free-src_BINARY_DIR} EXCLUDE_FROM_ALL)

include_directories(${r8brain-free-src_SOURCE_DIR})
