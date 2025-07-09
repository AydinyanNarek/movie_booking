## Movie‑Booking Service & SDK

A self‑contained C++ 17 gRPC back‑end for on‑line movie‑ticket booking, plus a header‑only SDK and a tiny CLI client. Everything can be built either locally **or** fully inside Docker. When running dist_release target the doxygen documentation will be generated automatically inside dist/docs folder. 

---

### Features

|                                                     |  ✔   |
| --------------------------------------------------- | :-:  |
| Modern C++ 17 code‑base (no raw pointers)           |  ✅  |
| gRPC + Protocol Buffers wire protocol               |  ✅  |
| In-memory repository (20 seats per theater)         |  ✅  |
| Thread-safe booking - **no double-assignments**     |  ✅  |
| Unit tests (Catch2) & integration smoke-test        |  ✅  |
| Single-image Docker build *(server + client + SDK)* |  ✅  |
| Conan 2 auto-boot-strapped package management       |  ✅  |
| **dist\_release** target -> ready-to-ship *dist/*   |  ✅  |
| **Doxygen + Graphviz** API reference docs           |  ✅  |
---

## Directory Layout (top‑level)

```
.
├── CMakeLists.txt        ← build script (auto‑boots Conan)
├── conanfile.txt         ← declarative dependencies
├── proto/booking.proto   ← gRPC / Protobuf schema
├── src/ …                ← domain & service code
├── grpc/                 ← BookingServiceImpl + server_main.cpp
├── cli/booking_cli.cpp   ← simple interactive CLI client
├── tests/                ← unit & integration tests
├── docker/Dockerfile     ← multi‑stage image (server + client)
├── tools/                ← helper CMake scripts (Docker & dist)
└── docs/                 ← Doxygen template (Doxyfile.in)
```

---

## 1. Local build (Linux / macOS / Windows MSVC)

> **Prerequisites**
> • CMake ≥ 3.23
> • Python 3 + `pip install conan==2.3.2`
> • Docker ≥ 28.3.0 + `Make sure that docker running and user have correct privilegies`
> • C++ 17 tool‑chain (GCC ≥ 9 / Clang ≥ 10 / MSVC 17.4)


```bash
# configure + bootstrap Conan
cmake -S . -B build

# compile + run unit tests
cmake --build build -j
ctest --test-dir build --output-on-failure

# install SDK + binaries
cmake --install build --prefix install   # -> install/{bin,lib,include}
```

### Run locally

```bash
# terminal 1 - start server
./install/bin/booking_server --host 0.0.0.0 --port 6000 &

# terminal 2 - CLI client
./install/bin/booking_client list-movies    --host 127.0.0.1 --port 6000
./install/bin/booking_client list-theaters  --movie 1
./install/bin/booking_client list-seats     --movie 1 --theater 101
./install/bin/booking_client book           --movie 1 --theater 101 --seat A3
./install/bin/booking_client list-seats     --movie 1 --theater 101
```

---

## 2. Build **inside Docker** (zero host deps)

```bash
# one‑shot build - produces image movie_booking:Release
cmake --build build --target docker            # or --config Release
```

Image contents:

* `/opt/movie_booking/bin/{booking_server,booking_client}`
* static library + headers (SDK)

### 2.1 Run (docker image)

```bash
docker run --rm -it --user root --entrypoint /bin/bash movie_booking:Release
Use the steps described in `Run locally` paraggraph
```

## 3. Create a distributable bundle

```bash
cmake --build build --target dist_release
```

Produces the following structure:

```
dist/
├── conanfile.txt
├── proto/booking.proto
├── docs/html/index.html        ← generated API docs
├── docker/BookingService-Release.tar
└── sdk/
    ├── bin/{booking_server,booking_client,tests}
    ├── include/…               ← headers (public + generated)
    └── lib/libmovie_booking.a  ← plus doxygen generated documentation
```

### How the customer uses it

**A)** Run the service quickly

```bash
cd dist
docker load -i docker/BookingService-Release.tar
docker run --rm -it --user root --entrypoint /bin/bash movie_booking:Release

#Now you are inside docker container, run the following commands
booking_server --host 0.0.0.0 --port 6000 &
booking_client list-movies    --host 127.0.0.1 --port 6000
booking_client list-theaters  --movie 1
booking_client list-seats     --movie 1 --theater 101
booking_client book           --movie 1 --theater 101 --seat A3
booking_client list-seats     --movie 1 --theater 101

```

**B)** Build a native client against the SDK

```bash
export CMAKE_PREFIX_PATH="$PWD/dist/sdk"
cmake -S my_app -B build -DCMAKE_PREFIX_PATH="$CMAKE_PREFIX_PATH"
cmake --build build
See examples folder demo project
```