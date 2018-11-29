cmake_minimum_required(VERSION 3.12)

include(FetchContent)

if(LIBSNDFILE_ROOT_DIR)
  # instructs FetchContent to not download or update but use the location instead
  set(FETCHCONTENT_SOURCE_DIR_LIBSNDFILE ${LIBSNDFILE_ROOT_DIR})
else()
  set(FETCHCONTENT_SOURCE_DIR_LIBSNDFILE "")
endif()

set(LIBSNDFILE_GIT_REPO "https://github.com/erikd/libsndfile" CACHE STRING "libsndfile git repository url" FORCE)
set(LIBSNDFILE_GIT_TAG b4bd397ca74f4c72b9cabaae66fef0c3d5a8c527 CACHE STRING "libsndfile git tag" FORCE)

FetchContent_Declare(libsndfile
      GIT_REPOSITORY    ${LIBSNDFILE_GIT_REPO}
      GIT_TAG           ${LIBSNDFILE_GIT_TAG}
      GIT_CONFIG        advice.detachedHead=false
      SOURCE_DIR        "${CMAKE_BINARY_DIR}/libsndfile"
      BINARY_DIR        "${CMAKE_BINARY_DIR}/libsndfile-build"
      CONFIGURE_COMMAND ""
      BUILD_COMMAND     ""
      INSTALL_COMMAND   ""
      TEST_COMMAND      ""
      )

FetchContent_GetProperties(libsndfile)

if(NOT libsndfile_POPULATED)

  if(FETCHCONTENT_SOURCE_DIR_LIBSNDFILE)
    message(STATUS "Using libsndfile from local ${FETCHCONTENT_SOURCE_DIR_LIBSNDFILE}")
  else()
    message(STATUS "Fetching libsndfile ${LIBSNDFILE_GIT_REPO}@${LIBSNDFILE_GIT_TAG}")
  endif()

  FetchContent_Populate(libsndfile)

endif()

set(LIBSNDFILE_ROOT_DIR ${libsndfile_SOURCE_DIR})
set(LIBSNDFILE_INCLUDE_DIR "${libsndfile_BINARY_DIR}/src")

function(libsndfile_build)
  set(BUILD_EXAMPLES OFF)
  set(BUILD_PROGRAMS OFF)
  set(BUILD_TESTING OFF)
  set(BUILD_REGTEST OFF)
  set(ENABLE_CPACK OFF)
  set(ENABLE_PACKAGE_CONFIG OFF)
  cmake_policy(SET CMP0063 NEW)
  # finally we include libsndfile itself
  add_subdirectory(${libsndfile_SOURCE_DIR} ${libsndfile_BINARY_DIR} EXCLUDE_FROM_ALL)
  # copying .hh for c++ support
  file(COPY "${libsndfile_SOURCE_DIR}/src/sndfile.hh" DESTINATION ${LIBSNDFILE_INCLUDE_DIR})
endfunction()

libsndfile_build()

include_directories(${LIBSNDFILE_INCLUDE_DIR})