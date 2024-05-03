#pragma once

#include "geo.h"

#include <string>
#include <vector>

namespace transport {

struct Stop {
    std::string name;
    geo::Coordinates coordinates;
};

struct Bus {
    std::string name;
    std::vector<const Stop*> route;
    bool is_round = false;
    size_t unique_stops_amount = 0;
    int geo_length = 0;
    
    int GetStopsAmount() const;
    double ComputeDirectRouteLenght() const;
    double ComputeCurvature() const;
};
} // namespace transport
