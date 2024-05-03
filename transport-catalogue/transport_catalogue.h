#pragma once

#include "domain.h"

#include <deque>
#include <functional>
#include <set>
#include <string_view>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace transport {

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
    
    void AddStop(Stop&& stop);
    void SetDistance(const Stop* from_name, const Stop* to_name, int distance);
    void AddBus(std::string_view name, const std::vector<std::string_view>& route, bool is_round);
    const Bus* GetBusInfo(std::string_view name) const;
    const Stop* GetStopInfo(std::string_view name) const;
    const std::deque<Stop>& GetStopsList() const;
    const std::deque<Bus>& GetRoutesList() const;
    int GetDistance(const Stop* from_name, const Stop* to_name) const;
    const std::set<std::string_view>* GetBusesListForStop(std::string_view name) const;

private:
    std::deque<Stop> stops_;
    std::deque<Bus> buses_;
    std::unordered_map<std::string_view, const Stop*> stops_index_;
    std::unordered_map<std::string_view, const Bus*> buses_index_;
    std::unordered_map<std::string_view, std::set<std::string_view>> stop_to_buses_index_;
    std::unordered_map<std::pair<const Stop*, const Stop*>, int,
                                 detail::PairPtrHasher, detail::PairComp> stops_distances_index_;

    void FillGeoLength(Bus& route) const;
};
} // namespace transport
