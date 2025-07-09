#include "booking/domain/Movie.hpp"

using namespace booking::domain;

Movie::Movie(Id id, std::string t, std::string d)
    : id_{id}, title_{std::move(t)}, description_{std::move(d)} {}
