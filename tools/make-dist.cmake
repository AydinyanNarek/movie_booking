# tools/make-dist.cmake
# Usage:  BUILD_TYPE=Debug  cmake -P tools/make-dist.cmake
#         cmake -P tools/make-dist.cmake            (defaults to Release)
cmake_minimum_required(VERSION 3.23)

# -------------------------------------------------------------------
# 0. configuration
# -------------------------------------------------------------------

set(BUILD_TYPE $ENV{BUILD_TYPE})
if(NOT BUILD_TYPE)    # default matches the image tag produced by make-dist
    set(BUILD_TYPE Release)
endif()

set(DIST_DIR "${CMAKE_SOURCE_DIR}/dist")
file(REMOVE_RECURSE      "${DIST_DIR}")
file(MAKE_DIRECTORY      "${DIST_DIR}")
file(MAKE_DIRECTORY      "${DIST_DIR}/docker")

# where the SDK is inside the runtime image  ←── only line that really changed
set(SDK_IN_IMAGE "/opt/movie_booking")   # must match Dockerfile !

# name for a short-lived helper container
set(TMP_CONT "movie_booking_tmp")

# -------------------------------------------------------------------
# 1. make sure no stale container is lying around
# -------------------------------------------------------------------
execute_process(
    COMMAND docker rm -f ${TMP_CONT}
    RESULT_VARIABLE _rm_rv
    OUTPUT_QUIET ERROR_QUIET) 

# -------------------------------------------------------------------
# 2. create an ephemeral container from the image we just built
# -------------------------------------------------------------------
execute_process(
    COMMAND docker create --name ${TMP_CONT} movie_booking:${BUILD_TYPE}
    OUTPUT_VARIABLE _cid OUTPUT_STRIP_TRAILING_WHITESPACE
    RESULT_VARIABLE rv)
if(rv)
    message(FATAL_ERROR "docker create failed (${rv})")
endif()

# -------------------------------------------------------------------
# 3. copy SDK out of the container
# -------------------------------------------------------------------
execute_process(
    COMMAND docker cp ${TMP_CONT}:${SDK_IN_IMAGE}/.  ${DIST_DIR}/sdk
    RESULT_VARIABLE rv)
if(rv)
    message(FATAL_ERROR "docker cp failed (${rv})")
endif()

# -------------------------------------------------------------------
# 4. remove the helper container
# -------------------------------------------------------------------
execute_process(COMMAND docker rm -f ${TMP_CONT})

# -------------------------------------------------------------------
# 5. save the runtime image itself
# -------------------------------------------------------------------
set(TAR_PATH "${DIST_DIR}/docker/BookingService-${BUILD_TYPE}.tar")
execute_process(
    COMMAND docker save movie_booking:${BUILD_TYPE} -o ${TAR_PATH}
    RESULT_VARIABLE rv)
if(rv)
    message(FATAL_ERROR "docker save failed (${rv})")
endif()

# -------------------------------------------------------------------
# 6. copy headers, proto files and the Conan recipe (optional)
# -------------------------------------------------------------------
file(COPY "${CMAKE_SOURCE_DIR}/proto"         DESTINATION "${DIST_DIR}")
file(COPY "${CMAKE_SOURCE_DIR}/conanfile.txt" DESTINATION "${DIST_DIR}")

message(STATUS "Distributable bundle created ->  ${DIST_DIR}")
