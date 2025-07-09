// tests/integration/GrpcClientSample.cpp
// ─────────────────────────────────────────────────────────────────────────────
// Functional + concurrency smoke-test for the Booking gRPC service.
//
//   1. ListMovies
//   2. ListTheaters(movie_id)
//   3. ListFreeSeats(movie_id, theater_id)
//   4. BookSeats(movie_id, theater_id, seats)         (multi-threaded test)
//   5. Re-query free seats to verify booking succeeded
//
// Exit code ≠ 0 if any RPC fails or if over-booking is detected.
// ─────────────────────────────────────────────────────────────────────────────

#include "transport/ChannelFactory.hpp"
#include "booking.grpc.pb.h"

#include <grpcpp/grpcpp.h>
#include <absl/container/flat_hash_set.h>

#include <algorithm>
#include <atomic>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

namespace {            // ─────────────────────────── helpers / CLI
// ---------------------------------------------------------------------------
struct Cmd {
    std::string host = "127.0.0.1";
    int         port = 50051;
    std::string ipc  = "/tmp/booking.sock";   // ignored on Windows
};

Cmd parse(int argc, char** argv)
{
    Cmd cfg;
    for (int i = 1; i < argc; ++i) {
        std::string arg{argv[i]};
        auto next = [&] {
            if (++i >= argc)
                throw std::runtime_error("no value for " + arg);
            return std::string{argv[i]};
        };
        if      (arg == "--host") cfg.host = next();
        else if (arg == "--port") cfg.port = std::stoi(next());
        else if (arg == "--ipc")  cfg.ipc  = next();
        else if (arg == "--help") {
            std::cout << "GrpcClientSample [--host <addr>] "
                         "[--port <n>] [--ipc <path>]\n";
            std::exit(0);
        } else {
            throw std::runtime_error("unknown option " + arg);
        }
    }
    return cfg;
}
// ---------------------------------------------------------------------------
std::string seatsToStr(const booking::SeatList& list)
{
    std::ostringstream oss;
    for (int i = 0; i < list.seats_size(); ++i) {
        if (i) oss << ", ";
        oss << list.seats(i).label();
    }
    return oss.str();
}

void dumpMovies(const booking::MovieList& m)
{
    for (const auto& mv : m.movies())
        std::cout << "  • [" << mv.id() << "] " << mv.title() << '\n';
    if (m.movies().empty()) std::cout << "  (none)\n";
}

void dumpTheaters(const booking::TheaterList& t)
{
    for (const auto& th : t.theaters())
        std::cout << "  • [" << th.id() << "] " << th.name() << '\n';
    if (t.theaters().empty()) std::cout << "  (none)\n";
}
// ---------------------------------------------------------------------------
// Run the whole scenario on one channel; return true on success
bool exerciseChannel(const std::string& lbl,
                     std::shared_ptr<grpc::Channel> ch)
{
    auto stub = booking::Booking::NewStub(std::move(ch));
    bool ok = true;

    // 1) ListMovies ----------------------------------------------------------
    grpc::ClientContext ctx1;
    booking::MovieList  movieList;
    if (!stub->ListMovies(&ctx1, booking::Empty{}, &movieList).ok()) {
        std::cerr << "[ " << lbl << " ] ListMovies RPC failed\n";
        return false;
    }
    std::cout << "\n[ " << lbl << " ] ListMovies:\n";
    dumpMovies(movieList);
    if (movieList.movies().empty()) return true;        // nothing to test

    const auto movieId = movieList.movies(0).id();

    // 2) ListTheaters --------------------------------------------------------
    grpc::ClientContext ctx2;
    booking::MovieId mid;  mid.set_id(movieId);
    booking::TheaterList theaterList;
    if (!stub->ListTheaters(&ctx2, mid, &theaterList).ok()) {
        std::cerr << "  ListTheaters RPC failed\n";
        return false;
    }
    dumpTheaters(theaterList);
    if (theaterList.theaters().empty()) return ok;

    const auto theaterId = theaterList.theaters(0).id();

    // 3) ListFreeSeats -------------------------------------------------------
    grpc::ClientContext ctx3;
    booking::TheaterReq tq;
    tq.set_movie_id(movieId);
    tq.set_theater_id(theaterId);
    booking::SeatList freeSeats;
    if (!stub->ListFreeSeats(&ctx3, tq, &freeSeats).ok()) {
        std::cerr << "  ListFreeSeats RPC failed\n";
        return false;
    }
    std::cout << "  free: " << seatsToStr(freeSeats) << '\n';
    if (freeSeats.seats().empty()) {
        std::cout << "  (no free seats, concurrency test skipped)\n";
        return ok;
    }

    // we'll try to book the first free seat
    const auto seatLabel = freeSeats.seats(0).label();

    // 4) Concurrency test - 4 threads compete for the SAME seat -------------
    constexpr int kThreads = 4;
    std::atomic<int> success{0};

    auto bookTask = [&] {
        grpc::ClientContext localCtx;
        booking::BookingReq br;
        br.set_movie_id(movieId);
        br.set_theater_id(theaterId);
        br.add_seats()->CopyFrom(freeSeats.seats(0));
        booking::BookingRep rep;
        if (stub->BookSeats(&localCtx, br, &rep).ok() && rep.success())
            ++success;
    };

    std::vector<std::thread> workers;
    for (int i = 0; i < kThreads; ++i) workers.emplace_back(bookTask);
    for (auto& t : workers) t.join();

    std::cout << "  concurrency booked=" << success << " (expected 1)\n";
    if (success != 1) { std::cerr << "  ERROR: over-booking detected!\n"; ok = false; }

    // 5) Verify the seat is gone -------------------------------------------
    grpc::ClientContext ctx4;
    booking::SeatList after;
    if (!stub->ListFreeSeats(&ctx4, tq, &after).ok()) {
        std::cerr << "  ListFreeSeats re-check failed\n";
        return false;
    }
    std::cout << "  after-booking free: " << seatsToStr(after) << '\n';

    const bool stillFree =
        std::any_of(after.seats().begin(), after.seats().end(),
                    [&](const booking::Seat& s){ return s.label() == seatLabel; });

    if (stillFree) {
        std::cerr << "  ERROR: seat " << seatLabel << " still appears free!\n";
        ok = false;
    }

    return ok;
}
// ---------------------------------------------------------------------------
} // unnamed namespace

// ─────────────────────────────────────────────────────────────────────────────
int main(int argc, char** argv)
{
    const Cmd cfg = parse(argc, argv);

    std::vector<std::pair<std::string, std::shared_ptr<grpc::Channel>>> chans;
    chans.emplace_back("TCP", transport::makeNetworkChannel(cfg.host, cfg.port));
#ifndef _WIN32
    if (!cfg.ipc.empty())
        chans.emplace_back("IPC", transport::makeLocalChannel(cfg.ipc));
#endif

    bool global_ok = true;
    for (auto& [lbl, ch] : chans)
        global_ok &= exerciseChannel(lbl, ch);

    return global_ok ? 0 : 1;
}
