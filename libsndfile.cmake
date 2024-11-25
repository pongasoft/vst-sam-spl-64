cmake_minimum_required(VERSION 3.12)

include(FetchContent)

set(LIBSNDFILE_GIT_REPO "https://github.com/erikd/libsndfile" CACHE STRING "libsndfile git repository url" FORCE)
set(LIBSNDFILE_GIT_TAG c11deaa04ec84161996824061f6d705970972e2e CACHE STRING "libsndfile git tag" FORCE)

if(LIBSNDFILE_ROOT_DIR)
  message(STATUS "Using libsndfile from local ${LIBSNDFILE_ROOT_DIR}")
  FetchContent_Populate(libsndfile
      QUIET
      SOURCE_DIR        "${LIBSNDFILE_ROOT_DIR}"
      BINARY_DIR        "${CMAKE_BINARY_DIR}/libsndfile-build"
  )
else()
  message(STATUS "Fetching libsndfile from ${LIBSNDFILE_GIT_REPO}/tree/${LIBSNDFILE_GIT_TAG}")
  FetchContent_Populate(libsndfile
      QUIET
      GIT_REPOSITORY    ${LIBSNDFILE_GIT_REPO}
      GIT_TAG           ${LIBSNDFILE_GIT_TAG}
      GIT_CONFIG        advice.detachedHead=false
      #      GIT_SHALLOW       true
      SOURCE_DIR        "${CMAKE_BINARY_DIR}/libsndfile"
      BINARY_DIR        "${CMAKE_BINARY_DIR}/libsndfile-build"
  )
endif()

set(LIBSNDFILE_ROOT_DIR ${libsndfile_SOURCE_DIR})
set(LIBSNDFILE_INCLUDE_DIR "${libsndfile_BINARY_DIR}/src")

function(libsndfile_build)
  option(BUILD_PROGRAMS "Build programs" OFF)
  option(BUILD_EXAMPLES "Build examples" OFF)
  option(BUILD_TESTING "Build examples" OFF)
  option(ENABLE_CPACK "Enable CPack support" OFF)
  option(ENABLE_PACKAGE_CONFIG "Generate and install package config file" OFF)
  option(BUILD_REGTEST "Build regtest" OFF)
  # finally we include libsndfile itself
  add_subdirectory(${libsndfile_SOURCE_DIR} ${libsndfile_BINARY_DIR} EXCLUDE_FROM_ALL)
  # copying .hh for c++ support
  file(COPY "${libsndfile_SOURCE_DIR}/src/sndfile.hh" DESTINATION ${LIBSNDFILE_INCLUDE_DIR})
endfunction()

libsndfile_build()

include_directories(${LIBSNDFILE_INCLUDE_DIR})
