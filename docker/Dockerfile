# docker/Dockerfile
# -----------------------------------------------------------------------------
# Build with:       docker build -t movie_booking:<type> \
#                       --build-arg BUILD_TYPE=<Debug|Release|…>  .
# Typical example:  docker build -t movie_booking:Release .
#
# Run server:       docker run --rm -p 50051:50051 movie_booking:Release
# Run client:       docker run --rm --entrypoint booking_client \
#                       movie_booking:Release list-movies
# -----------------------------------------------------------------------------

# syntax=docker/dockerfile:1.7
ARG BUILD_TYPE=Release                # picked up by every stage

############################
# 1. base tool-chain layer #
############################
FROM ubuntu:24.04 AS base
ARG BUILD_TYPE                        # <── must repeat to be visible here
ENV DEBIAN_FRONTEND=noninteractive

# APT & pip caches, BuildKit style
RUN --mount=type=cache,target=/var/cache/apt,sharing=locked \
    --mount=type=cache,target=/var/lib/apt,sharing=locked \
    apt-get update && \
    apt-get install -y --no-install-recommends \
        build-essential cmake ninja-build git \
        doxygen graphviz \
        python3-pip python3-venv && \
    rm -rf /var/lib/apt/lists/*

RUN --mount=type=cache,target=/root/.cache/pip,sharing=locked \
    PIP_BREAK_SYSTEM_PACKAGES=1 pip3 install "conan==2.3.2"

############################
# 2. deps  - Conan install #
############################
FROM base AS deps
ARG BUILD_TYPE                        # again!
WORKDIR /src

COPY conanfile.* CMakeLists.txt ./

RUN --mount=type=cache,target=/root/.conan2,sharing=locked \
    conan profile detect --name default --force && \
    conan install . -of build \
           --build=missing \
           -s build_type=${BUILD_TYPE}

############################
# 3. build - configure/make#
############################
FROM deps AS build
ARG BUILD_TYPE
WORKDIR /src

COPY . .

RUN --mount=type=cache,target=/root/.conan2,sharing=locked \
    cmake -S . -B build -GNinja \
          -DCMAKE_TOOLCHAIN_FILE=build/conan_toolchain.cmake \
          -DCMAKE_BUILD_TYPE=${BUILD_TYPE} && \
    cmake --build build -j$(nproc) && \
    cmake --build build --target docs && \
    cmake -E copy_directory build/docs /install/docs && \
    cmake --install build --prefix /install

########################################
# 4. runtime - slim image w/ full SDK  #
########################################
FROM ubuntu:24.04
# optional non-root user
RUN useradd -m booking
USER booking
WORKDIR /app

COPY --from=build /install /opt/movie_booking
ENV PATH="/opt/movie_booking/bin:${PATH}"

EXPOSE 50051/tcp

# — default = server, but you can override with --entrypoint
ENTRYPOINT ["booking_server"]
CMD []

# Helpful metadata
LABEL org.opencontainers.image.title="Movie-Booking (server+client)"
LABEL org.opencontainers.image.description="gRPC movie booking service and CLI client in one container"

