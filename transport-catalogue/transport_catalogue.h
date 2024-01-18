#pragma once

#include "geo.h"

#include <deque>
#include <set>
#include <string>
#include <string_view>
#include <unordered_map>
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
};

class TransportCatalogue {
public:
    
    void AddStop(std::string_view name, Coordinates&& coordinates);
    
    void AddBus(std::string_view name, const std::vector<std::string_view>& route);
    
    bool IsRouteExisted(std::string_view name) const;
    
    bool IsStopExisted(std::string_view name) const;
    
    size_t GetRouteStopsAmount(std::string_view name) const;
    
    size_t GetRouteUniqueStopsAmount(std::string_view name) const;
    
    double GetRouteLenght(std::string_view name) const;
    
    std::set<std::string_view> GetBusesListForStop(std::string_view name) const;
    
private:
    std::deque<Stop> stops_;
    std::deque<Bus> buses_;
    std::unordered_map<std::string_view, const Stop*> stops_index_;
    std::unordered_map<std::string_view, const Bus*> buses_index_;
    std::unordered_map<std::string_view, std::set<std::string_view>> stop_to_buses_index_;
};
} // namespace transport
