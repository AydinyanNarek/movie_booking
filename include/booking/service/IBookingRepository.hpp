#ifndef IBOOKING_REPOSITORY_HPP
#define IBOOKING_REPOSITORY_HPP

//  IBookingRepository.hpp
//  ---------------------------------------------------------------------------
//  Pure-virtual interface - persistence boundary for the booking domain.
//  Concrete implementations may store data in-memory, on disk, in a DB, etc.
//  The service layer only depends on this abstraction.
//
//  • All operations are synchronous and **thread-safe** guarantees are left to
//    the concrete implementation.
//  • Seats are represented by the lightweight value-type `domain::Seat`.
//
// ----------------------------------------------------------------------------
#include "booking/domain/Movie.hpp"
#include "booking/domain/Theater.hpp"
#include <memory>
#include <vector>

namespace booking::service {

/**
 * @interface IBookingRepository
 * @brief Persistence façade for the booking domain.
 *
 * An *application service* (e.g. @ref BookingManager) uses this interface to
 * query and mutate the underlying data store.  Implementations can range from
 * an in-memory map (unit tests) to a fully-fledged database adapter.
 *
 * @par Thread-safety
 *   The interface makes **no** guarantee.  Implementations must document
 *   whether concurrent calls are safe.
 */
class IBookingRepository
{
public:
    /// Virtual dtor — required for interface polymorphism
    virtual ~IBookingRepository() = default;

    // ── Queries ────────────────────────────────────────────────────────────

    /**
     * @brief Fetch every movie currently known to the system.
     * @return Vector of @ref booking::domain::Movie by value (cheap to copy).
     */
    [[nodiscard]]
    virtual std::vector<domain::Movie> movies() const = 0;

    /**
     * @brief Return all theaters that show a given movie.
     * @param id  Unique identifier of the movie.
     * @return Vector of **shared** immutable Theater handles.
     *
     * The caller receives `shared_ptr<const Theater>` so that multiple
     * consumers can safely observe the same @ref booking::domain::Theater
     * instance without accidental mutation.
     */
    [[nodiscard]]
    virtual std::vector<std::shared_ptr<const domain::Theater>>
        theaters(domain::Movie::Id id) const = 0;

    /**
     * @brief List seats that are still free for *one* movie/theater pair.
     * @param m  Movie identifier.
     * @param t  Theater identifier.
     * @return   Vector of free seats (possibly empty).
     */
    [[nodiscard]]
    virtual std::vector<domain::Seat>
        freeSeats(domain::Movie::Id m, domain::Theater::Id t) const = 0;

    // ── Command ────────────────────────────────────────────────────────────

    /**
     * @brief Atomically attempt to book a set of seats.
     *
     * @param m  Movie identifier.
     * @param t  Theater identifier.
     * @param s  Seat selection (must be non-empty).
     * @return   `true`  — all seats successfully booked.  
     *           `false` — at least one seat was already taken, **no** seat
     *                     is reserved (all-or-nothing semantics).
     *
     * Implementations are expected to enforce *transactional behaviour*: the
     * operation should either succeed for every requested seat or fail
     * completely, leaving the previous state intact.
     */
    virtual bool book(domain::Movie::Id               m,
                      domain::Theater::Id             t,
                      const std::vector<domain::Seat>& s) = 0;
};

} // namespace booking::service

#endif //IBOOKING_REPOSITORY_HPP