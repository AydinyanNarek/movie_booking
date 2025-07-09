// grpc/BookingServiceImpl.cpp
#include "BookingServiceImpl.hpp"
#include <absl/container/flat_hash_set.h>        // already shipped via gRPC
#include <algorithm>

/* convenient aliases (not exported) */
namespace  {
using booking::domain::Movie;
using booking::domain::Theater;
using booking::domain::Seat;
} // namespace

// ────────────────────────────────────────────────────────────────────────────
// 1) ListMovies
// ────────────────────────────────────────────────────────────────────────────
grpc::Status BookingServiceImpl::ListMovies(
        grpc::ServerContext*,
        const booking::Empty*,
        booking::MovieList* out)
{
    for (Movie const& m : mgr_->movies()) {
        auto* mm = out->add_movies();
        mm->set_id(m.id());
        mm->set_title(m.title());
        mm->set_description(m.desc());
    }
    return grpc::Status::OK;
}

// ────────────────────────────────────────────────────────────────────────────
// 2) ListTheaters
// ────────────────────────────────────────────────────────────────────────────
grpc::Status BookingServiceImpl::ListTheaters(
        grpc::ServerContext*,
        const booking::MovieId* in,
        booking::TheaterList* out)
{
    auto theaters = mgr_->theaters(in->id());
    if (theaters.empty())
        return grpc::Status(grpc::StatusCode::NOT_FOUND,
                            "movie id not found");

    for (auto const& t : theaters) {
        auto* tt = out->add_theaters();
        tt->set_id(t->id());
        tt->set_name(t->name());
    }
    return grpc::Status::OK;
}

// ────────────────────────────────────────────────────────────────────────────
// 3) ListFreeSeats
// ────────────────────────────────────────────────────────────────────────────
grpc::Status BookingServiceImpl::ListFreeSeats(
        grpc::ServerContext*,
        const booking::TheaterReq* req,
        booking::SeatList* out)
{
    auto seats = mgr_->freeSeats(req->movie_id(), req->theater_id());
    if (seats.empty())
        return grpc::Status(grpc::StatusCode::NOT_FOUND,
                            "movie/theater id not found");

    for (Seat const& s : seats) {
        auto* ss = out->add_seats();
        ss->set_index(s.index);
        ss->set_label(s.label);
    }
    return grpc::Status::OK;
}

// ────────────────────────────────────────────────────────────────────────────
// 4) BookSeats
// ────────────────────────────────────────────────────────────────────────────
grpc::Status BookingServiceImpl::BookSeats(
        grpc::ServerContext*,
        const booking::BookingReq* req,
        booking::BookingRep*      rep)
{
    // --- sanity: no duplicate seat labels in the request --------------------
    absl::flat_hash_set<std::string> uniq;
    std::vector<Seat> seats;
    seats.reserve(req->seats_size());

    for (auto const& s : req->seats()) {
        if (!uniq.insert(s.label()).second)
            return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT,
                                "duplicate seat label in request");

        seats.push_back({static_cast<std::uint8_t>(s.index()), s.label()});
    }

    if (seats.empty())
        return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT,
                            "no seats provided");

    const bool ok = mgr_->book(req->movie_id(),
                               req->theater_id(),
                               seats);

    rep->set_success(ok);
    if (!ok)
        return grpc::Status(grpc::StatusCode::ALREADY_EXISTS,
                            "one or more seats already booked");
    return grpc::Status::OK;
}
