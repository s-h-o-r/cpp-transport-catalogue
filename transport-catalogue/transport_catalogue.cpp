#include "transport_catalogue.h"

#include <algorithm>
#include <cassert>

namespace transport {

namespace detail {

size_t PairPtrHasher::operator()(const std::pair<const Stop*, const Stop*>& stops_pair) const {
    const auto [stop1, stop2] = stops_pair;
    size_t h1 = hasher(stop1->name);
    size_t h2 = hasher(stop2->name);
    return h1 ^ (h2 << 1);
}

bool PairComp::operator()(const std::pair<const Stop*, const Stop*>& lhs,
                const std::pair<const Stop*, const Stop*>& rhs) const {
    return lhs.first == rhs.first && lhs.second == rhs.second;
}

} // namespace transport::detail

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
    return route_length;
}

void TransportCatalogue::AddStop(std::string_view name, const Coordinates& coordinates) {
    stops_.push_back({ std::string(name), coordinates });
    stops_index_[stops_.back().name] = &stops_.back();
}

void TransportCatalogue::AddDistances(std::string_view name, const std::vector<std::pair<std::string_view, int>>& distances) {
    for (const auto& [next_stop, distance] : distances) {
        const Stop* stop1 = stops_index_.at(name);
        const Stop* stop2 = stops_index_.at(next_stop);
        stops_distances_index_[{stop1, stop2}] = distance;
    }
}

void TransportCatalogue::AddBus(std::string_view name, const std::vector<std::string_view>& route) {
    Bus bus;
    bus.name = std::string(name);
    for (auto stop : route) {
        assert(stops_index_.find(stop) != stops_index_.end());
        if (auto not_unique_stop_iter = find_if(bus.route.begin(), bus.route.end(),
                                                [&stop](const Stop* value) { return value->name == stop; });
            not_unique_stop_iter != bus.route.end()) {

            bus.route.push_back(*not_unique_stop_iter);
        } else {
            bus.route.push_back(stops_index_.at(stop));
            ++bus.unique_stops_amount;
        }
    }
    buses_.push_back(std::move(bus));
    const Bus* const added_bus_ptr = &buses_.back();
    buses_index_[buses_.back().name] = added_bus_ptr;
    for (const auto& stop : added_bus_ptr->route) {
        stop_to_buses_index_[stop->name].insert(added_bus_ptr->name);
    }
}

const Bus* TransportCatalogue::IsRouteExisted(std::string_view name) const {
    if (buses_index_.find(name) != buses_index_.end()) {
        return buses_index_.at(name);
    }
    return nullptr;
}

const Stop* TransportCatalogue::IsStopExisted(std::string_view name) const {
    if (stops_index_.find(name) != stops_index_.end()) {
        return stops_index_.at(name);
    }
    return nullptr;
}

std::set<std::string_view> TransportCatalogue::GetBusesListForStop(std::string_view name) const {
    if (stop_to_buses_index_.find(name) == stop_to_buses_index_.end()) {
        return {};
    }
    
    return stop_to_buses_index_.at(name);
}

std::pair<int, double> TransportCatalogue::GetGeoLengthAndCurvature(const Bus* route) const {
    int geo_length = ComputeGeoLength(route);
    return {static_cast<double>(geo_length), ComputeCurvature(geo_length, route->ComputeDirectRouteLenght())};
}

int TransportCatalogue::ComputeGeoLength(const Bus* route) const {
    int length = 0;
    const Stop* current_stop = nullptr;
    const Stop* next_stop = nullptr;
    for (const Stop* stop : route->route) {
        next_stop = stop;
        if (current_stop != nullptr) {
            std::pair<const Stop*, const Stop*> stop_to_stop = {current_stop, next_stop};
            auto distance_prt = stops_distances_index_.find(stop_to_stop);
            if (distance_prt == stops_distances_index_.end()) {
                stop_to_stop = {next_stop, current_stop};
                distance_prt = stops_distances_index_.find(stop_to_stop);
            }
            length += distance_prt->second;
        }
        current_stop = next_stop;
    }
    return length;
}

double TransportCatalogue::ComputeCurvature(double geo_length, double direct_length) const {
    return geo_length / direct_length;
}

} // namespace transport

