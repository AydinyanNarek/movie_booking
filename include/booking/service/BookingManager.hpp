#ifndef BOOKING_MANAGER_HPP
#define BOOKING_MANAGER_HPP

#include "IBookingRepository.hpp"
#include <memory>

namespace booking::service
{

/**
 * @file BookingManager.hpp
 * @brief **Thin façade** that exposes the booking use-cases while delegating
 *        actual data handling to an @ref booking::service::IBookingRepository.
 *
 * All business rules live in the repository implementation; this class adds
 * just a semantic layer so that other components (gRPC service, CLI, tests)
 * don’t need to know which concrete repository is in use
 * (in-memory, file-backed, SQL, …).
 *
 * It is therefore *stateless* and **cheap to copy / pass by value** - only the
 * shared-pointer to the repository is duplicated.
 */
class BookingManager
{
public:
    // ---------------------------------------------------------------------
    // Ctor
    // ---------------------------------------------------------------------
    /**
     * @brief Construct with a repository implementation.
     * @param repo Shared pointer to the data-access object; kept alive for
     *             the whole lifetime of the manager copy.
     */
    explicit BookingManager(std::shared_ptr<IBookingRepository> repo);

    // ---------------------------------------------------------------------
    // Read-only queries (forwarded 1-to-1)
    // ---------------------------------------------------------------------
    /// List all movies currently playing.
    [[nodiscard]] std::vector<domain::Movie> movies() const;

    /// List all theaters where @p id is screening.
    [[nodiscard]] std::vector<std::shared_ptr<const domain::Theater>>theaters(domain::Movie::Id id) const;

    /// Seat availability for a specific movie + hall.
    [[nodiscard]] std::vector<domain::Seat> freeSeats(domain::Movie::Id m, domain::Theater::Id t) const;

    // ---------------------------------------------------------------------
    // Mutation
    // ---------------------------------------------------------------------
    /**
     * @brief Atomically reserve a set of seats.
     * @return *true* if all seats were free and are now booked,
     *         *false* otherwise (no partial bookings).
     */
    bool book(domain::Movie::Id m, domain::Theater::Id t, const std::vector<domain::Seat>& s);

private:
    std::shared_ptr<IBookingRepository> repo_;   ///< Concrete DAO (shared).
};

} // namespace booking::service

#endif //BOOKING_MANAGER_HPP