#pragma once

#include "geo.h"

#include <cstddef>
#include <deque>
#include <functional>
#include <set>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

namespace transport {

struct Stop {
    std::string name;
    Coordinates coordinates;
};

struct Bus {
    std::string name;
    std::vector<const Stop*> route;
    size_t unique_stops_amount = 0;

    double ComputeDirectRouteLenght() const;
};

namespace detail {

struct PairPtrHasher {
    size_t operator()(const std::pair<const Stop*, const Stop*>& stops_pair) const;
    std::hash<std::string_view> hasher;
};

struct PairComp {
    bool operator()(const std::pair<const Stop*, const Stop*>& lhs,
                    const std::pair<const Stop*, const Stop*>& rhs) const;
};

} // namespace transport::detail

class TransportCatalogue {
public:
    
    void AddStop(std::string_view name, const Coordinates& coordinates);

    void AddDistances(std::string_view name, const std::vector<std::pair<std::string_view, int>>& distances);

    void AddBus(std::string_view name, const std::vector<std::string_view>& route);
    
    const Bus* IsRouteExisted(std::string_view name) const;

    const Stop* IsStopExisted(std::string_view name) const;
    
    std::set<std::string_view> GetBusesListForStop(std::string_view name) const;

    std::pair<int, double> GetGeoLengthAndCurvature(const Bus* route) const;

private:
    std::deque<Stop> stops_;
    std::deque<Bus> buses_;
    std::unordered_map<std::string_view, const Stop*> stops_index_;
    std::unordered_map<std::string_view, const Bus*> buses_index_;
    std::unordered_map<std::string_view, std::set<std::string_view>> stop_to_buses_index_;
    std::unordered_map<std::pair<const Stop*, const Stop*>, int, 
                                 detail::PairPtrHasher, detail::PairComp> stops_distances_index_;

    int ComputeGeoLength(const Bus* route) const;

    double ComputeCurvature(double geo_length, double direct_length) const;
};
} // namespace transport
