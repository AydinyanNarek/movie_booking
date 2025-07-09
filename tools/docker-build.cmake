# tools/docker-build.cmake
# Usage:  BUILD_TYPE=Debug  cmake -P tools/docker-build.cmake
#         cmake -P tools/docker-build.cmake            (defaults to Release)
cmake_minimum_required(VERSION 3.15)

set(BUILD_TYPE "$ENV{BUILD_TYPE}" CACHE STRING "")
if(NOT DEFINED ENV{BUILD_TYPE})
    set(BUILD_TYPE "Release")
else()
    set(BUILD_TYPE "$ENV{BUILD_TYPE}")
endif()

message(STATUS "Docker build with BUILD_TYPE=${BUILD_TYPE}")

# Ensure BuildKit caches are enabled
set(ENV{DOCKER_BUILDKIT} "1")
set(ENV{BUILDKIT_STEP_LOG_MAX_SIZE} "0")   # 0 = unlimited

execute_process(
    COMMAND docker build --progress=plain
            --build-arg BUILD_TYPE=${BUILD_TYPE}
            -t movie_booking:${BUILD_TYPE}
            -f ${CMAKE_SOURCE_DIR}/docker/Dockerfile
            ${CMAKE_SOURCE_DIR}
    RESULT_VARIABLE rv)

if(rv)
    message(FATAL_ERROR "Docker build failed (exit code ${rv})")
endif()
