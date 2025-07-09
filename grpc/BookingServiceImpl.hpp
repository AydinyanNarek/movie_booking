#ifndef BOOKING_SERVER_IMPL_HPP
#define BOOKING_SERVER_IMPL_HPP

#include "booking/service/BookingManager.hpp"
#include "booking.grpc.pb.h"
#include <grpcpp/grpcpp.h>
#include <memory>

/**
 * @file BookingServiceImpl.hpp
 * @brief Concrete gRPC service implementation that exposes the
 *        **movie-booking** domain to remote clients.
 *
 * This class wraps a shared instance of
 * `booking::service::BookingManager` and forwards each gRPC request to
 * the corresponding façade method. All heavy-lifting (validation,
 * concurrency control, business rules) lives inside the manager; the
 * service is a thin transport layer.
 */

 /**
  * @class BookingServiceImpl
  * @brief Implements the *booking.Booking* gRPC service generated from
  *        *booking.proto*.
  *
  * ```text
  * +--------------+       +---------------------+
  * | gRPC Client  | <--->  | BookingServiceImpl  |  <--->  BookingManager
  * +--------------+       +---------------------+
  *        │                         │
  *        │  protobuf messages      │  C++ domain objects
  *        ▼                         ▼
  *  (network/I/O)               (in-memory logic)
  * ```
  */
class BookingServiceImpl final : public booking::Booking::Service
{
public:
    /**
     * @brief Construct the service with an already-configured manager.
     * @param m Shared pointer to the `BookingManager` used to satisfy requests.
     *
     * The pointer is **not** copied; ownership is shared with the caller.
     */
    explicit BookingServiceImpl(std::shared_ptr<booking::service::BookingManager> m)
        : mgr_(std::move(m)) {}

    // ─────────────────────────────── RPC overrides ─────────────────────────

    /**
     * @brief Return the full list of movies currently “playing”.
     * @param ctx   gRPC server context (unused).
     * @param in    Empty request message.
     * @param out   Filled with repeated `Movie` messages on success.
     * @return `grpc::Status::OK` on success, or an error status on failure.
     */
    grpc::Status ListMovies(
        grpc::ServerContext*           ctx,
        const booking::Empty*          in,
        booking::MovieList*            out) override;

    /**
     * @brief Return all theaters that show the given movie.
     * @param ctx   gRPC server context.
     * @param in    Message with a single `movie_id` field.
     * @param out   Filled with repeated `Theater` messages.
     */
    grpc::Status ListTheaters(
        grpc::ServerContext*           ctx,
        const booking::MovieId*        in,
        booking::TheaterList*          out) override;

    /**
     * @brief Return all still-free seats for *movie + theatre* pair.
     * @param ctx   gRPC server context.
     * @param in    Request with `movie_id` and `theater_id`.
     * @param out   Filled with a repeated list of `Seat` messages.
     */
    grpc::Status ListFreeSeats(
        grpc::ServerContext*           ctx,
        const booking::TheaterReq*     in,
        booking::SeatList*             out) override;

    /**
     * @brief Atomically try to reserve the requested seats.
     * @param ctx   gRPC server context.
     * @param in    Booking request with IDs and seat list.
     * @param out   Reply with a single `success` boolean.
     *
     * The underlying manager guarantees **no double-booking** under
     * concurrent calls; a failed attempt returns `success = false`.
     */
    grpc::Status BookSeats(
        grpc::ServerContext*           ctx,
        const booking::BookingReq*     in,
        booking::BookingRep*           out) override;

private:
    /// Shared pointer to the business-logic façade.
    std::shared_ptr<booking::service::BookingManager> mgr_;
};

#endif //BOOKING_SERVER_IMPL_HPP