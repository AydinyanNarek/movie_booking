#ifndef SEAT_HPP
#define SEAT_HPP

#include <cstdint>
#include <string>

namespace booking::domain
{

/**
 * @file Seat.hpp
 * @brief Lightweight value-type that identifies a single seat inside a theater.
 *
 * A theater in this demo has a single row of **20 seats**. We store both
 * a numeric index (`0 … 19`) that is fast to compare and a human-readable
 * <tt>A&nbsp;N</tt> label (e.g. “A7”).  
 * The struct is a *Plain-Old-Data* aggregate: no virtuals, trivial copy/move.
 */

/**
 * @struct Seat
 * @brief Immutable record that denotes one physical seat.
 *
 * The numeric `index` is the <b>canonical key</b>; two seats compare equal if
 * their indices match, regardless of how the label was obtained.
 */
struct Seat
{
    /** @name Data members (public for POD semantics) */
    ///@{

    /// Zero-based index in the fixed 20-seat row (`0 … 19`).
    std::uint8_t index{};

    /// Human-readable label in the form <tt>A1 … A20</tt>.
    std::string  label;
    ///@}

    /** @name Convenience helpers */
    ///@{

    /**
     * @brief Factory that converts a numeric slot <tt>i</tt> to a `Seat`.
     * @param i Zero-based slot number.
     * @return  Seat with matching index and generated label.
     *
     * ```cpp
     * Seat s = Seat::fromIndex(5);   // -> index=5, label="A6"
     * ```
     */
    static Seat fromIndex(std::uint8_t i)
    {
        return { i, "A" + std::to_string(i + 1) };
    }

    /// Equality compares *only* the numeric key.
    [[nodiscard]] bool operator==(const Seat& o) const noexcept
    {
        return index == o.index;
    }
    ///@}
};

} // namespace booking::domain

#endif //SEAT_HPP