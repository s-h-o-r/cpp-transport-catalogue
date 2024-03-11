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

void TransportCatalogue::AddStop(Stop&& stop) {
    stops_.push_back(std::move(stop));
    stops_index_[stops_.back().name] = &stops_.back();
    stop_to_buses_index_[stops_.back().name];
}

void TransportCatalogue::SetDistance(const Stop* from_name, const Stop* to_name, int distance) {
    stops_distances_index_[{from_name, to_name}] = distance;
}

void TransportCatalogue::AddBus(std::string_view name, const std::vector<std::string_view>& route, bool is_round) {
    Bus bus;

    bus.is_round = is_round;
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
    FillGeoLength(bus);
    buses_.push_back(std::move(bus));
    const Bus* const added_bus_ptr = &buses_.back();
    buses_index_[buses_.back().name] = added_bus_ptr;
    for (const auto& stop : added_bus_ptr->route) {
        stop_to_buses_index_[stop->name].insert(added_bus_ptr->name);
    }
}

const Bus* TransportCatalogue::GetBusInfo(std::string_view name) const {
    if (buses_index_.find(name) != buses_index_.end()) {
        return buses_index_.at(name);
    }
    return nullptr;
}

const Stop* TransportCatalogue::GetStopInfo(std::string_view name) const {
    if (stops_index_.find(name) != stops_index_.end()) {
        return stops_index_.at(name);
    }
    return nullptr;
}

std::vector<const Bus*> TransportCatalogue::GetRoutesList() const {
    std::vector<const Bus*> routes_list;
    for (const auto& bus : buses_) {
        routes_list.push_back(&bus);
    }
    return routes_list;
}

int TransportCatalogue::GetDistance(const Stop* from_name, const Stop* to_name) const {
    auto distance_ptr = stops_distances_index_.find({from_name, to_name});
    if (distance_ptr == stops_distances_index_.end()) {
        distance_ptr = stops_distances_index_.find({to_name, from_name});
    }
    if (distance_ptr == stops_distances_index_.end()) {
        return 0;
    }

    return distance_ptr->second;
}

const std::set<std::string_view>* TransportCatalogue::GetBusesListForStop(std::string_view name) const {
    if (stop_to_buses_index_.find(name) == stop_to_buses_index_.end()) {
        return nullptr;
    }
    
    return &stop_to_buses_index_.at(name);
}

void TransportCatalogue::FillGeoLength(Bus& route) const {
    const Stop* current_stop = nullptr;
    const Stop* next_stop = nullptr;
    for (const Stop* stop : route.route) {
        next_stop = stop;
        if (current_stop != nullptr) {
            route.geo_length += GetDistance(current_stop, next_stop);
        }
        current_stop = next_stop;
    }

    if (!route.is_round) {
        for (auto iter = route.route.rbegin() + 1; iter != route.route.rend(); ++iter) {
            next_stop = *iter;
            if (current_stop != nullptr) {
                route.geo_length += GetDistance(current_stop, next_stop);
            }
            current_stop = next_stop;
        }
    }
}

} // namespace transport

