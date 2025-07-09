#ifndef CHANNEL_FACTORY_HPP
#define CHANNEL_FACTORY_HPP
//  ChannelFactory.hpp
//  ---------------------------------------------------------------------------
//  Helpers that build gRPC client-side channels for network (TCP) or local
//  domain-socket (IPC) transport.
//  ---------------------------------------------------------------------------
#include "Endpoints.hpp"
#include <grpcpp/grpcpp.h>
#include <memory>
#include <string>

namespace transport {

/**
 * @brief Create a *network* gRPC channel (TCP).
 *
 * Convenience wrapper that resolves a TCP endpoint via
 * Endpoints::tcp() and constructs a channel with insecure credentials
 * (sufficient for local development / demos).
 *
 * @param host Remote host, defaults to `"localhost"`.
 * @param port TCP port, defaults to `50051`.
 * @return A shared `grpc::Channel` ready for use with a generated Stub.
 *
 * @note Blocking dial-up is deferred until the first RPC.
 */
inline std::shared_ptr<grpc::Channel>
makeNetworkChannel(const std::string& host = "localhost", int port = 50051)
{
    return grpc::CreateChannel(
        Endpoints::tcp(host, port),
        grpc::InsecureChannelCredentials());
}

/**
 * @brief Create a *local* (IPC) gRPC channel over a Unix-domain socket.
 *
 * @param path Filesystem path of the socket (default: `/tmp/booking.sock`).
 * @return A shared channel, or `nullptr` on Windows where UDS is unsupported.
 *
 * The helper is a no-op stub on Windows to allow cross-platform compilation.
 */
inline std::shared_ptr<grpc::Channel>
makeLocalChannel(const std::string& path = "/tmp/booking.sock")
{
#ifndef _WIN32
    return grpc::CreateChannel(
        Endpoints::ipc(path),
        grpc::InsecureChannelCredentials());
#else
    (void)path;
    return nullptr;             // IPC not available on Windows
#endif
}

} // namespace transport
#endif //CHANNEL_FACTORY_HPP