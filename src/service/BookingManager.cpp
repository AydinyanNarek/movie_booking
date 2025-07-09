#include "booking/service/BookingManager.hpp"

using namespace booking;

service::BookingManager::BookingManager(
        std::shared_ptr<service::IBookingRepository> repo)
    : repo_(std::move(repo)) {}

std::vector<domain::Movie>
service::BookingManager::movies() const
{
    return repo_->movies();
}

std::vector<std::shared_ptr<const domain::Theater>>
service::BookingManager::theaters(domain::Movie::Id id) const
{
    return repo_->theaters(id);
}

std::vector<domain::Seat>
service::BookingManager::freeSeats(domain::Movie::Id m,
                                   domain::Theater::Id t) const
{
    return repo_->freeSeats(m, t);
}

bool service::BookingManager::book(domain::Movie::Id m,
                                   domain::Theater::Id t,
                                   const std::vector<domain::Seat>& s)
{
    return repo_->book(m, t, s);
}
