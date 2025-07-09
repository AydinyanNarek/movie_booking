#ifndef ENDPOINTS_HPP
#define ENDPOINTS_HPP

//  Endpoints.hpp
//  ---------------------------------------------------------------------------
//  Tiny helpers that build endpoint strings accepted by gRPC:
//
//    • tcp()  -> "host:port"         («ipv4», "[ipv6]", or hostname)
//    • ipc()  -> "unix:/tmp/…sock"   (Unix-domain socket URI)
//
//  These helpers allow the server & client code to stay platform-agnostic.
//  ---------------------------------------------------------------------------
#include <string>

namespace transport {

/**
 * @brief Namespace-style struct that groups endpoint helpers.
 */
struct Endpoints
{
    /**
     * @brief Return a *TCP* endpoint in the `"host:port"` form.
     *
     * Handles IPv4, IPv6 (bracketed), and plain hostnames exactly as gRPC
     * expects.
     *
     * @param host Remote host (default `"127.0.0.1"`).
     * @param port TCP port  (default `50051`).
     * @return Concatenated `"host:port"` string.
     */
    static std::string tcp(const std::string& host = "127.0.0.1",
                           int                port = 50051)
    {
        return host + ":" + std::to_string(port);
    }

    /**
     * @brief Return a *Unix-domain socket* URI understood by gRPC.
     *
     * On non-Windows platforms the helper prepends `"unix:"` which is
     * required by the gRPC C++ transport layer.  
     * On Windows (where UDS is unsupported) the function returns an empty
     * string so that callers can gracefully skip IPC.
     *
     * @param path Filesystem path to the socket
     *             (default `"/tmp/booking.sock"`).
     * @return `"unix:/path"` on POSIX, empty string on Windows.
     */
    static std::string ipc(const std::string& path = "/tmp/booking.sock")
    {
#ifdef _WIN32
        (void)path;              // IPC unavailable on Windows
        return {};
#else
        return "unix:" + path;   // gRPC requires the "unix:" prefix
#endif
    }
};

} // namespace transport

#endif //ENDPOINTS_HPP