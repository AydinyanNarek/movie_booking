#ifndef THEATER_HPP
#define THEATER_HPP

#include "Seat.hpp"
#include <bitset>
#include <cstdint>
#include <mutex>
#include <string>
#include <vector>

namespace booking::domain
{

/**
 * @file Theater.hpp
 * @brief Thread-safe seat-allocation model for a small (20-seat) theater hall.
 *
 * The object owns the seat map and ensures **atomic** booking operations under
 * contention (multiple client requests / threads).  
 * Copying is *prohibited* — there is exactly one truth for a given hall —
 * but the class is <em>move-enabled</em> so that containers can re-locate it.
 */

/**
 * @class Theater
 * @brief A single screening hall with fixed-size seating capacity.
 *
 * **Thread-safety contract**
 * | Member function     | Concurrency guarantee                    |
 * |---------------------|-------------------------------------------|
 * | `freeSeats()`       | safe ­concurrent reads                   |
 * | `tryBook()`         | atomic reservation, serialised via mutex |
 *
 * Internally we keep a `std::bitset` where *bit == 1* means **occupied**.
 */
class Theater
{
public:
    // ---------------------------------------------------------------------
    // Public types & constants
    // ---------------------------------------------------------------------
    /// Total seats in this demo theater (**fixed**).
    static constexpr std::size_t kCapacity = 20;

    /// Stable identifier type used by the service layer / clients.
    using Id = std::uint32_t;

    // ---------------------------------------------------------------------
    // Rule-of-Five - copy disabled, move enabled
    // ---------------------------------------------------------------------
    /**
     * @brief Construct a hall with given id + human name.
     * @param id_   Numeric database / API id.
     * @param name_ Friendly display name (e.g. *Cinema A — Hall 1*).
     */
    explicit Theater(Id id_, std::string name_);

    Theater(Theater&&) noexcept;
    Theater& operator=(Theater&&) noexcept;

    Theater(const Theater&)            = delete;
    Theater& operator=(const Theater&) = delete;

    // ---------------------------------------------------------------------
    // Inspectors
    // ---------------------------------------------------------------------
    /// Immutable numeric id.
    [[nodiscard]] Id id() const noexcept                { return id_; }

    /// Human-readable hall label.
    [[nodiscard]] const std::string& name() const noexcept { return name_; }

    /**
     * @brief Return the current list of *free* seats.
     *
     * Runtime O(N) where N==`kCapacity`; negligible for 20 seats.
     * The method locks `mtx_` only for the very short copy of the bitset,
     * then releases it so that expensive string conversions happen unlocked.
     */
    [[nodiscard]] std::vector<Seat> freeSeats() const;

    /**
     * @brief Attempt to book the supplied seats atomically.
     * @param seats Vector of seat descriptors (`index` field is key).
     * @retval true   all requested seats were available **and are now taken**.
     * @retval false  at least one seat was already occupied - no change.
     *
     * The whole operation executes under a single mutex - either no bit flips
     * or all flips succeed.
     */
    bool tryBook(const std::vector<Seat>& seats);

private:
    // ---------------------------------------------------------------------
    // Data members
    // ---------------------------------------------------------------------
    Id                     id_;          ///< Stable id.
    std::string            name_;        ///< Display label.
    mutable std::mutex     mtx_;         ///< Serialises seat map access.
    std::bitset<kCapacity> occupancy_;   ///< 1 == *taken*.
};

} // namespace booking::domain

#endif //THEATER_HPP