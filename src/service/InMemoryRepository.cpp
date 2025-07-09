/**
 *  @file InMemoryRepository.cpp
 *  @brief Simple thread-safe, in-memory implementation of
 *         booking::service::IBookingRepository.
 *
 *  The repository keeps all data in STL containers, guarded by a
 *  `std::shared_mutex`:
 *  * many concurrent read-only operations are allowed (`shared_lock`)
 *  * writers (`book()`) obtain an exclusive lock
 *
 *  @note
 *  * **No** persistence layer - everything lives only for the life-time
 *    of the process.
 *  * Capacity of every theatre is fixed to @c Theater::kCapacity (20).
 *  * The initial dataset is hard-coded in #seed().
 */

#include "booking/service/IBookingRepository.hpp"
#include "booking/domain/Movie.hpp"
#include "booking/domain/Theater.hpp"

#include <shared_mutex>
#include <unordered_map>
#include <memory>

using booking::domain::Movie;
using booking::domain::Theater;
using booking::domain::Seat;

namespace booking::service {

/* ---------------------------------------------------------------------------*
 *  InMemoryRepository                                                         *
 * ---------------------------------------------------------------------------*/

/**
 *  @class InMemoryRepository
 *  @brief Concrete implementation of IBookingRepository that stores everything
 *         in RAM.
 *
 *  All public member functions acquire a lock internally, therefore callers
 *  can use the same instance from multiple threads without additional
 *  synchronisation.
 */
class InMemoryRepository final : public IBookingRepository
{
public:
    /** Constructs the repo and populates it with three movies / four theaters. */
    InMemoryRepository() { seed(); }

    // ------------------------------------------------------------------ I/F --
    /// @copydoc IBookingRepository::movies()
    std::vector<Movie> movies() const override
    {
        std::shared_lock read{rw_};

        std::vector<Movie> result;
        result.reserve(db_.size());
        for (auto& kv : db_) {
            result.push_back(kv.second.movie);
        }
        return result;
    }

    /// @copydoc IBookingRepository::theaters()
    std::vector<std::shared_ptr<const Theater>> theaters(Movie::Id m) const override
    {
        std::shared_lock read{rw_};

        std::vector<std::shared_ptr<const Theater>> result;
        const auto it = db_.find(m);
        if (it == db_.end()) {
            return result;                         // unknown movie -> empty list
        }

        for (auto& kv : it->second.theaters) {
            result.push_back(kv.second);
        }
        return result;
    }

    /// @copydoc IBookingRepository::freeSeats()
    std::vector<Seat> freeSeats(Movie::Id m, Theater::Id t) const override
    {
        std::shared_lock read{rw_};
        return db_.at(m).theaters.at(t)->freeSeats();
    }

    /// @copydoc IBookingRepository::book()
    bool book(Movie::Id m, Theater::Id t,
              const std::vector<Seat>& seats) override
    {
        std::shared_lock read{rw_};

        const auto mIt = db_.find(m);
        if (mIt == db_.end()) {                    // unknown movie
            return false;
        }

        const auto tIt = mIt->second.theaters.find(t);
        if (tIt == mIt->second.theaters.end()) {   // unknown theatre
            return false;
        }

        return tIt->second->tryBook(seats);
    }

private:
    /** Populates #db_ with a fixed test dataset. */
    void seed()
    {
        Movie inter{1, "Interstellar"};
        Movie inception{2, "Inception"};

        db_[inter.id()].movie    = inter;
        db_[inception.id()].movie = inception;

        db_[1].theaters.emplace(
            101, std::make_shared<Theater>(101, "CinemaA-Hall1"));
        db_[1].theaters.emplace(
            102, std::make_shared<Theater>(102, "CinemaA-Hall2"));
        db_[2].theaters.emplace(
            201, std::make_shared<Theater>(201, "CinemaB-Hall1"));
    }

private:
    /* ------------------------------------------------------------------ impl */
    /** Simple aggregate that bundles one movie with all its theaters. */
    struct Entry {
        Movie movie;
        std::unordered_map<Theater::Id, std::shared_ptr<Theater>> theaters;
    };

    mutable std::shared_mutex            rw_;   ///< readers/writer lock
    std::unordered_map<Movie::Id, Entry> db_;   ///< whole dataset
};

/* ---------------------------------------------------------------------------*
 *  Factory helper                                                             *
 * ---------------------------------------------------------------------------*/

/**
 *  @brief Creates a thread-safe in-memory repository instance.
 *
 *  @return `std::shared_ptr<IBookingRepository>`
 */
std::shared_ptr<IBookingRepository> makeInMemoryRepository()
{
    return std::make_shared<InMemoryRepository>();
}

} // namespace booking::service
