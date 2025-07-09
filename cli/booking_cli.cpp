// cli/booking_client.cpp
//
// Small CLI talking to the Booking gRPC service.
//
//   booking_client list-movies                        [... global opts]
//   booking_client list-theaters --movie 2
//   booking_client list-seats  --movie 2 --theater 201
//   booking_client book        --movie 2 --theater 201 --seat A7[,A8…]
//
// Global options may go *anywhere*:
//   --host <addr>   (default 127.0.0.1)
//   --port <num>    (default 50051)
//   --ipc  <path>   (default /tmp/booking.sock, ignored on Windows)
// -------------------------------------------------------------------------

/**
*    # list all movies via TCP
*    booking_client --host 127.0.0.1 --port 50051 list-movies
*
*    # list halls for movie 1 over the Unix-domain socket
*    booking_client --ipc /tmp/booking.sock list-theaters --movie 1
*
*    # see free seats, then try to book two of them
*    booking_client list-seats --movie 2 --theater 201
*    booking_client book --movie 2 --theater 201 --seat A1,A2
*
*    # built-in help
*    booking_client --help 
**/

#include "transport/ChannelFactory.hpp"
#include "booking.grpc.pb.h"
#include <grpcpp/grpcpp.h>

#include <getopt.h>     // POSIX   (see fallback below for MSVC)
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <vector>
#include <cstring>

#ifdef _WIN32
/* ------------------------------------------------------------------------
   Minimal getopt_long() shim for MSVC / Windows.  Good enough for this CLI.
   (If you build with MinGW or clang-cl + Posix layer you already have it.)
   ----------------------------------------------------------------------*/
#include <cctype>
static int   optind   = 1, opterr = 1, optopt;
static char* optarg   = nullptr;
struct option { const char* name; int has_arg; int* flag; int val; };
static int getopt_long(int argc, char* const argv[],
                       const char*, const option* longopts, int* longidx)
{
    if (optind >= argc) return -1;
    char* arg = argv[optind];
    if (arg[0] != '-' || arg[1] != '-') return -1;           // only --long
    std::string key = arg + 2;
    optind++;
    for (int i = 0; longopts[i].name; ++i) {
        if (key == longopts[i].name) {
            if (longopts[i].has_arg) {
                if (optind >= argc) { optopt = longopts[i].val; return '?'; }
                optarg = argv[optind++];
            }
            if (longidx) *longidx = i;
            return longopts[i].val;
        }
    }
    return '?';
}
#endif // _WIN32
/* --------------------------------------------------------------------- */

struct Config {
    std::string cmd;                // list-movies, list-theaters, …
    uint32_t    movie   = 0;
    uint32_t    theater = 0;
    std::vector<std::string> seats; // for booking

    std::string host = "127.0.0.1";
    int         port = 50051;
    std::string ipc  = "/tmp/booking.sock";
};

static void usage(const char* prog)
{
    std::cout <<
R"(Usage:
  )" << prog << R"( <command> [options]

Commands
  list-movies
  list-theaters   --movie <id>
  list-seats      --movie <id> --theater <id>
  book            --movie <id> --theater <id> --seat <label>[,<label>...]

Global connection options
  --host  <addr>   (default 127.0.0.1)
  --port  <num>    (default 50051)
  --ipc   <path>   (default /tmp/booking.sock, Linux only)

Misc
  -h, --help       Show this help and exit
)";
}

/* ---------- argument parsing ------------------------------------------------ */
static Config parse(int argc, char** argv)
{
    Config cfg;
    int longidx = 0;
    static option opts[] = {
        {"movie",   required_argument, nullptr, 'm'},
        {"theater", required_argument, nullptr, 't'},
        {"seat",    required_argument, nullptr, 's'},
        {"host",    required_argument, nullptr, 'H'},
        {"port",    required_argument, nullptr, 'P'},
        {"ipc",     required_argument, nullptr, 'I'},
        {"help",    no_argument,       nullptr, 'h'},
        {nullptr,   0,                 nullptr,  0 }
    };

    /* first pass just to grab global flags independent of position */
    optind = 1;                     // reset (for shim / POSIX alike)
    while (true) {
        int c = getopt_long(argc, argv, "m:t:s:H:P:I:h", opts, &longidx);
        if (c == -1) break;
        switch (c) {
            case 'm': cfg.movie   = std::stoul(optarg);            break;
            case 't': cfg.theater = std::stoul(optarg);            break;
            case 's': {                                            // comma split
                std::stringstream ss(optarg); std::string tok;
                while (std::getline(ss, tok, ',')) cfg.seats.push_back(tok);
                break;
            }
            case 'H': cfg.host = optarg;                           break;
            case 'P': cfg.port = std::stoi(optarg);                break;
            case 'I': cfg.ipc  = optarg;                           break;
            case 'h': usage(argv[0]); std::exit(0);
            default : usage(argv[0]); std::exit(1);
        }
    }

    /* the remaining non-option arg(s) are the command */
    if (optind >= argc) { usage(argv[0]); std::exit(1); }
    cfg.cmd = argv[optind];

    return cfg;
}

/* ---------- helpers --------------------------------------------------------- */
static std::shared_ptr<grpc::Channel> makeChannel(const Config& c)
{
#ifndef _WIN32
    if (!c.ipc.empty())
        return transport::makeLocalChannel(c.ipc);
#endif
    return transport::makeNetworkChannel(c.host, c.port);
}

int main(int argc, char** argv)
try {
    Config cfg = parse(argc, argv);
    auto stub  = booking::Booking::NewStub(makeChannel(cfg));
    grpc::ClientContext ctx;

    if (cfg.cmd == "list-movies") {
        booking::Empty req; booking::MovieList resp;
        if (!stub->ListMovies(&ctx, req, &resp).ok())
            throw std::runtime_error("ListMovies RPC failed");

        for (auto& m : resp.movies())
            std::cout << m.id() << '\t' << m.title() << '\n';
    }
    else if (cfg.cmd == "list-theaters") {
        if (!cfg.movie) { std::cerr << "--movie required\n"; return 1; }
        booking::MovieId mid; mid.set_id(cfg.movie);
        booking::TheaterList resp;
        if (!stub->ListTheaters(&ctx, mid, &resp).ok())
            throw std::runtime_error("ListTheaters RPC failed");

        for (auto& t : resp.theaters())
            std::cout << t.id() << '\t' << t.name() << '\n';
    }
    else if (cfg.cmd == "list-seats") {
        if (!cfg.movie || !cfg.theater) {
            std::cerr << "--movie and --theater required\n"; return 1; }
        booking::TheaterReq req;
        req.set_movie_id(cfg.movie);
        req.set_theater_id(cfg.theater);
        booking::SeatList resp;
        if (!stub->ListFreeSeats(&ctx, req, &resp).ok())
            throw std::runtime_error("ListFreeSeats RPC failed");

        for (auto& s : resp.seats()) std::cout << s.label() << ' ';
        std::cout << '\n';
    }
    else if (cfg.cmd == "book") {
        if (!cfg.movie || !cfg.theater || cfg.seats.empty()) {
            std::cerr << "--movie --theater --seat required\n"; return 1; }
        booking::BookingReq req;
        req.set_movie_id(cfg.movie);
        req.set_theater_id(cfg.theater);
        for (auto& lbl : cfg.seats) {
            auto* seat = req.add_seats();
            seat->set_label(lbl);
            int n = 0;
            if (lbl.size() > 1 && std::isdigit(lbl[1]))
                n = std::stoi(lbl.substr(1));  // "A17" -> 17
            seat->set_index(n - 1);    
        }

        booking::BookingRep rep;
        if (!stub->BookSeats(&ctx, req, &rep).ok())
            throw std::runtime_error("BookSeats failed");
        std::cout << (rep.success() ? "booked\n" : "booking failed\n");
    }
    else {
        std::cerr << "Unknown command '" << cfg.cmd << "'\n";
        usage(argv[0]);
        return 1;
    }

    return 0;
}
catch (const std::exception& ex) {
    std::cerr << "error: " << ex.what() << '\n';
    return 1;
}
