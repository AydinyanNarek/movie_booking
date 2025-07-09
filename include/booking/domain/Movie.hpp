#ifndef MOVIE_HPP
#define MOVIE_HPP

#include <cstdint>
#include <string>

namespace booking::domain
{

/**
 * @file Movie.hpp
 * @brief Plain-old data class that represents a single movie title in the
 *        booking domain.
 *
 * The class is intentionally *light-weight & trivially copyable*: it holds
 * only identifiers and descriptive strings.  Any mutable state (e.g. box-office
 * stats) belongs elsewhere.
 */

/**
 * @class Movie
 * @brief Immutable value-type describing a movie currently available for
 *        booking.
 *
 * A `Movie` is identified by an <b>integer primary-key</b> (`Id`) that is
 * unique inside the service.  The rest is read-only metadata that user-interfaces
 * may display.
 *
 * ```text
 * ┌─────────────┐
 * │  Movie      │
 * ├─────────────┤
 * │ id          │ 32-bit unsigned
 * │ title       │ UTF-8 title
 * │ description │ Free-form text (optional)
 * └─────────────┘
 * ```
 *
 * @note The class provides <em>trivial</em> getters only; once constructed a
 *       movie instance never changes.
 */
class Movie
{
public:
    /// Unsigned 32-bit primary key used throughout the service.
    using Id = std::uint32_t;

    /** @name Constructors */
    ///@{

    /// Default-construct an *invalid* movie with `id == 0`.
    Movie() : id_{0} {}

    /**
     * @brief Construct a fully-specified movie.
     * @param id           Unique identifier (≠ 0).
     * @param title        Human-readable title (UTF-8).
     * @param description  Optional synopsis / tagline. May be empty.
     */
    Movie(Id id, std::string title, std::string description = {});
    ///@}

    /** @name Read-only accessors */
    ///@{

    /// Numerical identifier (never changes after construction).
    [[nodiscard]] Id id() const noexcept            { return id_; }

    /// Movie title.
    [[nodiscard]] const std::string& title() const noexcept { return title_; }

    /// Optional extended description / synopsis.
    [[nodiscard]] const std::string& desc()  const noexcept { return description_; }
    ///@}

private:
    Id          id_{0};
    std::string title_;
    std::string description_;
};

} // namespace booking::domain

#endif //MOVIE_HPP
