#pragma once

#include "domain.h"
#include "graph.h"
#include "router.h"

#include <deque>
#include <functional>
#include <optional>
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

class RoutesGraph {
public:
    RoutesGraph(const TransportCatalogue& db);
    std::pair<size_t, size_t> GetStopVertexes(const Stop* stop) const;
    const graph::DirectedWeightedGraph<double>* GetGraph() const;
    const EdgeInfo* GetEdgeInfo(graph::EdgeId edge_id) const;
    double GetEdgeWeight(graph::EdgeId edge_id) const;
    void BuildGraph(const RouteSettings& settings);
    std::optional<graph::Router<double>::RouteInfo> BuildRoute(const Stop* from, const Stop* to) const;

private:
    const TransportCatalogue& db_;
    std::optional<graph::DirectedWeightedGraph<double>> routes_graph_ = std::nullopt;
    std::optional<graph::Router<double>> router_ = std::nullopt;
    std::unordered_map<const Stop*, std::pair<size_t, size_t>> vertex_index_;
    std::unordered_map<graph::EdgeId, EdgeInfo> edges_index_;

    void AddVertexes(const RouteSettings& settings);
    void AddRouteEdges(const RouteSettings& settings);
    void BuildRouter();

};

} // namespace transport
