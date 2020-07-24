cmake_minimum_required(VERSION 3.12)

include(FetchContent)

if(R8BRAIN-FREE-SRC_ROOT_DIR)
  # instructs FetchContent to not download or update but use the location instead
  set(FETCHCONTENT_SOURCE_DIR_R8BRAIN-FREE-SRC ${R8BRAIN-FREE-SRC_ROOT_DIR})
else()
  set(FETCHCONTENT_SOURCE_DIR_R8BRAIN-FREE-SRC "")
endif()

set(R8BRAIN-FREE-SRC_GIT_REPO "https://github.com/pongasoft/r8brain-free-src" CACHE STRING "r8brain-free-src git repository url" FORCE)
set(R8BRAIN-FREE-SRC_GIT_TAG version-4.6.yp CACHE STRING "r8brain-free-src git tag" FORCE)

FetchContent_Declare(r8brain-free-src
    GIT_REPOSITORY    ${R8BRAIN-FREE-SRC_GIT_REPO}
    GIT_TAG           ${R8BRAIN-FREE-SRC_GIT_TAG}
    GIT_CONFIG        advice.detachedHead=false
    GIT_SHALLOW       true
    SOURCE_DIR        "${CMAKE_BINARY_DIR}/r8brain-free-src"
    BINARY_DIR        "${CMAKE_BINARY_DIR}/r8brain-free-src-build"
    CONFIGURE_COMMAND ""
    BUILD_COMMAND     ""
    INSTALL_COMMAND   ""
    TEST_COMMAND      ""
    )

FetchContent_GetProperties(r8brain-free-src)

if(NOT r8brain-free-src_POPULATED)

  if(FETCHCONTENT_SOURCE_DIR_R8BRAIN-FREE-SRC)
    message(STATUS "Using r8brain-free-src from local ${FETCHCONTENT_SOURCE_DIR_R8BRAIN-FREE-SRC}")
  else()
    message(STATUS "Fetching r8brain-free-src ${R8BRAIN-FREE-SRC_GIT_REPO}/tree/${R8BRAIN-FREE-SRC_GIT_TAG}")
  endif()

  FetchContent_Populate(r8brain-free-src)

endif()

add_subdirectory(${r8brain-free-src_SOURCE_DIR} ${r8brain-free-src_BINARY_DIR} EXCLUDE_FROM_ALL)

include_directories(${r8brain-free-src_SOURCE_DIR})
