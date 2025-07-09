// grpc/server_main.cpp
#include "BookingServiceImpl.hpp"
#include "transport/Endpoints.hpp"
#include <booking/service/IBookingRepository.hpp>
#include <booking/service/BookingManager.hpp>

#include <grpcpp/server_builder.h>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

/* factory declared in InMemoryRepository.cpp */
namespace booking::service {
std::shared_ptr<IBookingRepository> makeInMemoryRepository();
}

// ────────────────────────────────────────────────────────────────────────────
// Minimal CLI parser (no external deps)
// Usage:
//   booking_server [--host 0.0.0.0] [--port 50051] [--ipc /tmp/booking.sock]
// ────────────────────────────────────────────────────────────────────────────
struct Cmd {
    std::string host  = "0.0.0.0";
    int         port  = 50051;
    std::string ipc   =
#ifdef _WIN32
        "";                // IPC off by default on Windows
#else
        "/tmp/booking.sock";
#endif
};

Cmd parse(int argc, char** argv)
{
    Cmd cfg;
    for (int i = 1; i < argc; ++i) {
        const std::string arg{argv[i]};
        auto next = [&] { if (i + 1 >= argc) throw std::runtime_error("missing value for " + arg); return std::string{argv[++i]}; };

        if      (arg == "--host" || arg == "-h") cfg.host = next();
        else if (arg == "--port" || arg == "-p") cfg.port = std::stoi(next());
        else if (arg == "--ipc"  || arg == "-i") cfg.ipc  = next();
        else if (arg == "--help") {
            std::cout <<
              "booking_server [options]\n"
              "  --host, -h  <addr>   Bind address (default 0.0.0.0)\n"
              "  --port, -p  <num>    TCP port     (default 50051)\n"
              "  --ipc,  -i  <path>   Unix-domain socket path (empty to disable)\n";
            std::exit(0);
        }
        else throw std::runtime_error("unknown option " + arg);
    }
    return cfg;
}

int main(int argc, char** argv)
try {
    const Cmd cfg = parse(argc, argv);

    auto repo = booking::service::makeInMemoryRepository();
    auto mgr  = std::make_shared<booking::service::BookingManager>(repo);
    BookingServiceImpl svc{mgr};

#ifndef _WIN32
    if (!cfg.ipc.empty()) std::filesystem::remove(cfg.ipc);
#endif

    grpc::ServerBuilder builder;
    builder.AddListeningPort(
        transport::Endpoints::tcp(cfg.host, cfg.port),
        grpc::InsecureServerCredentials());

#ifndef _WIN32
    if (!cfg.ipc.empty()) {
        const std::string uri = "unix:" + cfg.ipc;      // <-- add prefix
        builder.AddListeningPort(uri, grpc::InsecureServerCredentials());
    }
#endif

    builder.RegisterService(&svc);

    auto server = builder.BuildAndStart();
    if (!server) {
        std::cerr << "Failed to start gRPC server\n";
        return 1;
    }
    
    std::cout << "Booking server up - TCP " << cfg.host << ':' << cfg.port;
#ifndef _WIN32
    if (!cfg.ipc.empty()) std::cout << " + IPC " << cfg.ipc;
#endif
    std::cout << '\n';
    server->Wait();
}
catch (std::exception& e) {
    std::cerr << "error: " << e.what() << '\n';
    return 1;
}
