#include "transport_catalogue.h"

#include <algorithm>
#include <cassert>

namespace transport {

double Bus::ComputeRouteLenght() const {
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
} // namespace transport

