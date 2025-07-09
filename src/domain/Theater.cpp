#include "booking/domain/Theater.hpp"

using namespace booking::domain;

/* ─── ctor ──────────────────────────────────────────────────────────────── */
Theater::Theater(Id id, std::string nm)
    : id_{id}, name_{std::move(nm)} {}

/* ─── move ctor ─────────────────────────────────────────────────────────── */
Theater::Theater(Theater&& other) noexcept {
    std::scoped_lock lk{other.mtx_};
    id_        = other.id_;
    name_      = std::move(other.name_);
    occupancy_ = other.occupancy_;
}

/* ─── move assign ───────────────────────────────────────────────────────── */
Theater& Theater::operator=(Theater&& other) noexcept {
    if (this == &other) return *this;
    std::scoped_lock lk{mtx_, other.mtx_};
    id_        = other.id_;
    name_      = std::move(other.name_);
    occupancy_ = other.occupancy_;
    return *this;
}

std::vector<Seat> Theater::freeSeats() const
{
    std::scoped_lock lk{mtx_};
    std::vector<Seat> v;
    for (std::size_t i = 0; i < kCapacity; ++i)
        if (!occupancy_.test(i))
            v.push_back(Seat::fromIndex(static_cast<std::uint8_t>(i)));
    return v;
}

bool Theater::tryBook(const std::vector<Seat>& seats)
{
    std::scoped_lock lk{mtx_};

    // a) validate indices first - reject out-of-range requests
    for (const auto& s : seats) {
        if (s.index >= kCapacity) { 
            return false;
        }
    }

    // b) reject if *any* seat already taken
    for (const auto& s : seats) {
        if (occupancy_.test(s.index)) { 
            return false;
        }
    }

    // c) all good -> reserve
    for (const auto& s : seats) {
        occupancy_.set(s.index);
    }
    return true;
}
