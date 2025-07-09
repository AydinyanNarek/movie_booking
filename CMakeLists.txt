cmake_minimum_required(VERSION 3.23)
project(movie_booking VERSION 1.0 LANGUAGES CXX)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "None" "Debug" "Release")
endif()

##############################################################################
#  Auto-bootstrap Conan (first configure only)
##############################################################################
if(NOT EXISTS "${CMAKE_BINARY_DIR}/conan_toolchain.cmake")

    # 1. Locate Conan and create default profile
    find_program(CONAN_CMD conan REQUIRED)
    execute_process(COMMAND "${CONAN_CMD}" profile detect --name default --force)

    # 2. Install deps -> generate conan_toolchain.cmake
    execute_process(
        COMMAND "${CONAN_CMD}" install
                "${CMAKE_SOURCE_DIR}"
                -of "${CMAKE_BINARY_DIR}"
                --build=missing
                -s build_type=${CMAKE_BUILD_TYPE})

    # 3. *Immediately* load the tool-chain **before** any find_package()
    include("${CMAKE_BINARY_DIR}/conan_toolchain.cmake" OPTIONAL RESULT_VARIABLE _ok)
    if(NOT _ok)
        message(FATAL_ERROR "Failed to include generated Conan tool-chain")
    endif()
endif()

##############################################################################
# Dependencies via CMakeDeps
##############################################################################
find_package(Protobuf REQUIRED CONFIG)
find_package(gRPC     REQUIRED CONFIG)
find_package(Catch2   REQUIRED CONFIG)

##############################################################################
# Protobuf & gRPC code-gen
##############################################################################
set(PROTO_FILES   ${CMAKE_CURRENT_SOURCE_DIR}/proto/booking.proto)

protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${PROTO_FILES})

set(GRPC_PLUGIN "$<TARGET_FILE:gRPC::grpc_cpp_plugin>")
protobuf_generate(
    LANGUAGE grpc
    GENERATE_EXTENSIONS .grpc.pb.h .grpc.pb.cc
    PLUGIN   "protoc-gen-grpc=${GRPC_PLUGIN}"
    PROTOS   ${PROTO_FILES}
    OUT_VAR  GRPC_SRCS)

include_directories(${CMAKE_CURRENT_BINARY_DIR})   # for generated *.pb.h

##############################################################################
# Core library
##############################################################################
add_library(${PROJECT_NAME} STATIC
    ${PROTO_SRCS} ${GRPC_SRCS}
    src/domain/Movie.cpp
    src/domain/Seat.cpp
    src/domain/Theater.cpp
    src/service/BookingManager.cpp
    src/service/InMemoryRepository.cpp
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO
    CXX_VISIBILITY_PRESET hidden
)

target_compile_options(movie_booking
  PRIVATE

  # =========== GCC / Clang (Linux - macOS) ===========
  $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>:
      -Wall
      -Wextra
      -Wformat=2
      -Wold-style-cast
      -Wnon-virtual-dtor
      -Wuseless-cast
      -Wfloat-equal
  >

  # =========== MSVC (Windows) ========================
  $<$<CXX_COMPILER_ID:MSVC>:
      /W4            # roughly -Wall
      /permissive-   # strict standard-conformance
      /EHsc          # proper exception-handling model
      /bigobj        # large generated files (protobuf)
  >

  # =========== Optimisation / Debug ==================
  $<$<CONFIG:Debug>:-O0 -g>                         # GCC/Clang
  $<$<AND:$<NOT:$<CONFIG:Debug>>,$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>>:-O3>
  $<$<AND:$<NOT:$<CONFIG:Debug>>,$<CXX_COMPILER_ID:MSVC>>:/O2>
)

target_include_directories(movie_booking
    PUBLIC
        # ---- visible while *building* this project ------------------------
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/proto>
        # ---- visible to *consumers* after “make install” ------------------
        $<INSTALL_INTERFACE:include>
        $<INSTALL_INTERFACE:include/generated>)

target_link_libraries(movie_booking
    PUBLIC protobuf::libprotobuf
           gRPC::grpc++)

##############################################################################
# Export as relocatable CMake package
##############################################################################
include(CMakePackageConfigHelpers)
set(export_dest lib/cmake/movie_booking)

# (a) targets file - automatically generated
install(TARGETS movie_booking
        EXPORT   movie_bookingTargets
        ARCHIVE  DESTINATION lib
        LIBRARY  DESTINATION lib
        RUNTIME  DESTINATION bin)

install(EXPORT movie_bookingTargets
        FILE      movie_bookingTargets.cmake
        NAMESPACE movie_booking::
        DESTINATION ${export_dest})

configure_package_config_file(
    ${CMAKE_SOURCE_DIR}/cmake/movie_bookingConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/movie_bookingConfig.cmake
    INSTALL_DESTINATION ${export_dest})

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/movie_bookingConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion)

install(FILES
        ${CMAKE_CURRENT_BINARY_DIR}/movie_bookingConfig.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/movie_bookingConfigVersion.cmake
        DESTINATION ${export_dest})

##############################################################################
# Applications & tests (unchanged)
##############################################################################
enable_testing()

add_executable(booking_server
    grpc/BookingServiceImpl.cpp grpc/server_main.cpp)
target_link_libraries(booking_server PRIVATE movie_booking)

add_executable(booking_client cli/booking_cli.cpp)
target_link_libraries(booking_client PRIVATE movie_booking)

file(GLOB UNIT_TESTS tests/unit/*.cpp)
add_executable(unit_tests ${UNIT_TESTS})
target_link_libraries(unit_tests PRIVATE movie_booking Catch2::Catch2WithMain)
add_test(NAME unit COMMAND unit_tests)

add_executable(integration_tests tests/integration/GrpcClientSample.cpp)
target_link_libraries(integration_tests PRIVATE movie_booking)

##############################################################################
# Header & generated-header install
##############################################################################
install(DIRECTORY include/                          DESTINATION include)
install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/proto DESTINATION include/generated
        FILES_MATCHING PATTERN "*.pb.h")

install(TARGETS booking_server booking_client RUNTIME DESTINATION bin)
install(TARGETS unit_tests integration_tests RUNTIME DESTINATION bin/tests)

##############################################################################
#  Helper targets - docker / dist_release (kept exactly as before)
##############################################################################
add_custom_target(docker
    COMMAND ${CMAKE_COMMAND} -P ${CMAKE_SOURCE_DIR}/tools/docker-build.cmake
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Building movie_booking:latest *entirely inside Docker*")

# ─────────────────────────────────────────────────────────────────────────────
#  Documentation (Doxygen) - always create a 'docs' target
# ─────────────────────────────────────────────────────────────────────────────
option(BUILD_DOCS "Generate API reference with Doxygen" ON)

if(BUILD_DOCS)
    find_package(Doxygen QUIET COMPONENTS dot)

    if(DOXYGEN_FOUND)
        # -- real docs target ------------------------------------------------
        set(DOXYFILE_IN  "${CMAKE_SOURCE_DIR}/docs/Doxyfile.in")
        set(DOXYFILE_OUT "${CMAKE_BINARY_DIR}/Doxyfile")

        configure_file(${DOXYFILE_IN} ${DOXYFILE_OUT} @ONLY)

        add_custom_target(docs
            COMMAND "${DOXYGEN_EXECUTABLE}" "${DOXYFILE_OUT}"
            COMMENT "Generating C++ API documentation with Doxygen"
            VERBATIM)
    else()
        # -- stub so downstream targets never fail ---------------------------
        add_custom_target(docs
            COMMENT "Doxygen / Graphviz not found - skipping docs generation")
    endif()
else()
    add_custom_target(docs
        COMMENT "BUILD_DOCS=OFF - documentation disabled")
endif()

add_custom_target(dist_release
    # 1) Docker image
    COMMAND ${CMAKE_COMMAND} -P ${CMAKE_SOURCE_DIR}/tools/docker-build.cmake
            -D BUILD_TYPE=${CMAKE_BUILD_TYPE}

    # 2) extract /install, save image, copy proto & README
    COMMAND ${CMAKE_COMMAND} -P ${CMAKE_SOURCE_DIR}/tools/make-dist.cmake
            -D BUILD_TYPE=${CMAKE_BUILD_TYPE}

    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/dist/sdk/docs/html
            ${CMAKE_SOURCE_DIR}/dist/docs/html
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    USES_TERMINAL
    COMMENT "Creating distributable bundle under dist/")

add_dependencies(dist_release docs)