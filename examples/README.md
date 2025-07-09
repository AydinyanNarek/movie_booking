# Native Movie-Booking Client (sample)

This tiny console application shows how a *third-party* project can:

1.  Pull in the **movie_booking** SDK (headers + static library) that ships
    inside `dist/sdk`, **without** needing gRPC / Protobuf sources.
2.  Call the gRPC service to list movies, theaters, seats, or book a seat.

---

## 1 Prerequisites

* CMake ≥ 3.23  
* A C++17-capable compiler (GCC ≥ 9, Clang ≥ 10, MSVC ≥ 2019, …)  
* The **SDK folder** produced by `cmake --build build --target dist_release`,
  extracted at `<PROJECT_ROOT>/dist/sdk`.

> The SDK already contains **`libmovie_booking.a` + headers** and a
> relocatable CMake package (`movie_bookingConfig.cmake`).  
> No external Conan/CMake work is required for the sample.

---

## 2 Build

```bash
# From <PROJECT_ROOT>
export CMAKE_PREFIX_PATH="$PWD/dist/sdk"

cmake -S examples/native_client -B examples/native_client/build
cmake --build examples/native_client/build -j
