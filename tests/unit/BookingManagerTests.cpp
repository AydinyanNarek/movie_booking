//  BookingManagerTests.cpp
//  ───────────────────────────────────────────────────────────────────────────
//  Unit-tests for the in-memory BookingManager + repository.
//  Build is automatic - it’s already picked up by the CMake glob in
//  tests/unit/*.cpp
//  ───────────────────────────────────────────────────────────────────────────
#define CATCH_CONFIG_MAIN
#include <catch2/catch_test_macros.hpp>
#include <atomic>
#include <thread>
#include <future>
#include "booking/service/BookingManager.hpp"
#include "booking/service/IBookingRepository.hpp"

namespace booking::service {
    std::shared_ptr<IBookingRepository> makeInMemoryRepository();
}

// ────────────────────────────────────────────────────────────────────────────
// 1. Single-seat booking should be atomic
// ────────────────────────────────────────────────────────────────────────────
TEST_CASE("Atomic booking")
{
    booking::service::BookingManager mgr{booking::service::makeInMemoryRepository()};

    // First attempt succeeds …
    REQUIRE( mgr.book(1, 101, {{0, "A1"}}) );

    // … second attempt must fail.
    REQUIRE_FALSE( mgr.book(1, 101, {{0, "A1"}}) );
}

// ────────────────────────────────────────────────────────────────────────────
// 2. Two threads racing for the *same* seat - exactly one wins
// ────────────────────────────────────────────────────────────────────────────
TEST_CASE("Concurrent race")
{
    booking::service::BookingManager mgr{booking::service::makeInMemoryRepository()};

    std::atomic<int> winners{0};
    auto task = [&] { if (mgr.book(1, 101, {{1, "A2"}})) ++winners; };

    std::thread t1(task), t2(task);
    t1.join(); t2.join();

    REQUIRE(winners == 1);
}

// ────────────────────────────────────────────────────────────────────────────
// 3. Booking *different* seats concurrently -> all should succeed
// ────────────────────────────────────────────────────────────────────────────
TEST_CASE("Concurrent independent bookings")
{
    booking::service::BookingManager mgr{booking::service::makeInMemoryRepository()};

    auto f1 = std::async(std::launch::async,
                         [&]{ return mgr.book(2, 201, {{0,"A1"}}); });

    auto f2 = std::async(std::launch::async,
                         [&]{ return mgr.book(2, 201, {{5,"A6"}}); });

    REQUIRE( f1.get() );
    REQUIRE( f2.get() );
    REQUIRE_FALSE( mgr.freeSeats(2, 201).empty() );
}

// ────────────────────────────────────────────────────────────────────────────
// 4. Free-seat list shrinks after a successful booking
// ────────────────────────────────────────────────────────────────────────────
TEST_CASE("Free-seats list updates")
{
    booking::service::BookingManager mgr{booking::service::makeInMemoryRepository()};
    const auto before = mgr.freeSeats(1,101).size();

    REQUIRE( mgr.book(1,101,{{2,"A3"}}) );

    const auto after  = mgr.freeSeats(1,101).size();
    REQUIRE( after == before - 1 );
}

// ────────────────────────────────────────────────────────────────────────────
// 5. Booking with an *invalid index* should fail gracefully
// ────────────────────────────────────────────────────────────────────────────
TEST_CASE("Out-of-range seat is rejected")
{
    booking::service::BookingManager mgr{booking::service::makeInMemoryRepository()};

    // Index 25 is beyond Theater::kCapacity (20)
    REQUIRE_FALSE( mgr.book(1,101,{{25,"A26"}}) );
}

// ────────────────────────────────────────────────────────────────────────────
// 6. Unknown movie / theater IDs - booking must fail
// ────────────────────────────────────────────────────────────────────────────
TEST_CASE("Non-existent movie / theater")
{
    booking::service::BookingManager mgr{booking::service::makeInMemoryRepository()};

    REQUIRE_FALSE( mgr.book(/*movie*/999, /*theater*/101, {{0,"A1"}}) );
    REQUIRE_FALSE( mgr.book(/*movie*/1,   /*theater*/999, {{0,"A1"}}) );
}

// ────────────────────────────────────────────────────────────────────────────
// 7. Exhaustive booking until capacity reached
// ────────────────────────────────────────────────────────────────────────────
TEST_CASE("Book all seats then reject")
{
    using namespace booking::domain;
    booking::service::BookingManager mgr{booking::service::makeInMemoryRepository()};

    // Book A1 … A20
    for (std::uint8_t i = 0; i < Theater::kCapacity; ++i)
        REQUIRE( mgr.book(1,101,{ Seat::fromIndex(i) }) );

    // Nothing left:
    REQUIRE( mgr.freeSeats(1,101).empty() );

    // Any further attempt must fail
    REQUIRE_FALSE( mgr.book(1,101,{{0,"A1"}}) );
}
