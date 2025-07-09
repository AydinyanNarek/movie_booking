cmake_minimum_required(VERSION 3.23)
project(native_client LANGUAGES CXX)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release")
endif()

list(APPEND CMAKE_PREFIX_PATH "$ENV{CMAKE_PREFIX_PATH}")
# -------------------------------------------------------------
#   Tell CMake where the SDK lives  (e.g.   export CMAKE_PREFIX_PATH=$PWD/dist/sdk)
# -------------------------------------------------------------
if (NOT CMAKE_PREFIX_PATH)
    message(FATAL_ERROR "Set CMAKE_PREFIX_PATH to the extracted SDK path (dist/sdk)")
endif()

# -------------------------------------------------------------
#   Run Conan ONCE to fetch protobuf & gRPC that the SDK expects
#     - we simply point Conan to the recipe that ships with the SDK
# -------------------------------------------------------------
set(SDK_CONANFILE "${CMAKE_PREFIX_PATH}/../conanfile.txt")

if (NOT EXISTS "${CMAKE_BINARY_DIR}/conan_toolchain.cmake")
    # Pick the current build type (default Release)
    if(NOT CMAKE_BUILD_TYPE)
        set(CONAN_BUILD_TYPE Release)
    else()
        set(CONAN_BUILD_TYPE ${CMAKE_BUILD_TYPE})
    endif()

    message(STATUS "Running Conan install for ${CONAN_BUILD_TYPE}")
    execute_process(
        COMMAND conan install "${SDK_CONANFILE}"
                --output-folder "${CMAKE_BINARY_DIR}"
                --build=missing
                -s build_type=${CONAN_BUILD_TYPE}
        RESULT_VARIABLE rv)
    if(rv AND NOT rv EQUAL 0)
        message(FATAL_ERROR "Conan install failed (exit code ${rv})")
    endif()

    # Let find_package() see the *.cmake files produced by CMakeDeps
    list(PREPEND CMAKE_PREFIX_PATH "${CMAKE_BINARY_DIR}")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# -------------------------------------------------------------
#    Locate the SDK (this cascades into protobuf / gRPC via
#     find_dependency() calls inside movie_bookingConfig.cmake)
# -------------------------------------------------------------
find_package(movie_booking REQUIRED CONFIG)

add_executable(native_client main.cpp)
target_link_libraries(native_client PRIVATE movie_booking::movie_booking)
