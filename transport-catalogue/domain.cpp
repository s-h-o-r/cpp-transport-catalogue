#include "domain.h"

namespace transport {

int Bus::GetStopsAmount() const {
    if (is_round) {
        return static_cast<int>(route.size());
    }
    return (static_cast<int>(route.size()) * 2 - 1);
}

double Bus::ComputeDirectRouteLenght() const {
    double route_length = 0.0;
    const Stop* from = nullptr;
    for (const Stop* to : route) {
        if (from == nullptr) {
            from = to;
            continue;
        }
        route_length += ComputeDistance(from->coordinates, to->coordinates);
        from = to;
    }
    if (!is_round) {
        return route_length * 2;
    }
    return route_length;
}

double Bus::ComputeCurvature() const {
    return geo_length / ComputeDirectRouteLenght();
}
} //namespace transport
