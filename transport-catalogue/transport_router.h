#pragma once

#include "graph.h"
#include "router.h"
#include "transport_catalogue.h"

#include <optional>
#include <memory>

#define KPH_TO_MPS_SPEED_COEF 1000. / 60.

namespace graph {

struct RouteSettings {
    int bus_wait_time = 0;
    int bus_velocity = 0;
};

class RoutesGraph {
public:
    struct EdgeInfo {
        const transport::Stop* from;
        const transport::Stop* to;
        const transport::Bus* bus;
        int span_count = 0;
        double weight = 0.;
    };

    struct Route {
        double weight = 0;
        std::vector<const EdgeInfo*> edges_info;
    };

    explicit RoutesGraph(const transport::TransportCatalogue& db, const RouteSettings& settings);

    std::optional<Route>
    BuildRoute(const transport::Stop* from, const transport::Stop* to) const;

private:
    const transport::TransportCatalogue& db_;
    std::unique_ptr<DirectedWeightedGraph<double>> routes_graph_;
    std::unique_ptr<Router<double>> router_ = nullptr;
    std::unordered_map<const transport::Stop*, std::pair<size_t, size_t>> vertex_index_;
    std::unordered_map<EdgeId, EdgeInfo> edges_index_;

    std::pair<size_t, size_t> GetStopVertexes(const transport::Stop* stop) const {
        return vertex_index_.at(stop);
    }

    const EdgeInfo* GetEdgeInfo(EdgeId edge_id) const;
    double GetEdgeWeight(EdgeId edge_id) const;
    void AddVertexes(const RouteSettings& settings);
    void AddRouteEdges(const RouteSettings& settings);
    void BuildGraph(const RouteSettings& settings);
    void BuildRouter();
};
} // namespace graph
