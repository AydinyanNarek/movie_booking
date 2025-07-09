// example/native_client/main.cpp
#include "transport/ChannelFactory.hpp"
#include <proto/booking.grpc.pb.h>
#include <grpcpp/grpcpp.h>
#include <iostream>

int main()
{
    /*  assumes the booking_server runs on localhost:50051   */
    auto channel = transport::makeNetworkChannel("127.0.0.1", 50051);
    auto stub    = booking::Booking::NewStub(channel);

    booking::Empty      req;
    booking::MovieList  resp;
    grpc::ClientContext ctx;

    auto status = stub->ListMovies(&ctx, req, &resp);
    if (!status.ok()) {
        std::cerr << "RPC error: " << status.error_message() << '\n';
        return 1;
    }

    std::cout << "Movies now playing:\n";
    for (const auto& m : resp.movies())
        std::cout << "  • [" << m.id() << "] " << m.title() << '\n';
    if (resp.movies().empty())
        std::cout << "  (none)\n";
    return 0;
}
