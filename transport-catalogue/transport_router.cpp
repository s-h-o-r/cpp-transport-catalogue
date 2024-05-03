#include "transport_router.h"

using namespace std::literals;

namespace graph {
RoutesGraph::RoutesGraph(const transport::TransportCatalogue& db, const RouteSettings& settings)
    : db_(db) {
    BuildGraph(settings);
    BuildRouter();
}

std::optional<RoutesGraph::Route>
RoutesGraph::BuildRoute(const transport::Stop* from, const transport::Stop* to) const {
    if (!router_) {
        throw std::logic_error("Router doesn't exist yet."s);
    }
    size_t vertex_from = GetStopVertexes(from).first;
    size_t vertex_to = GetStopVertexes(to).first;
    auto router_info = router_->BuildRoute(vertex_from, vertex_to);
    if (!router_info) {
        return std::nullopt;
    }

    std::vector<const EdgeInfo*> edges_info;
    for (graph::EdgeId edge_id : router_info->edges) {
        edges_info.push_back(GetEdgeInfo(edge_id));
    }

    return Route{router_info->weight, edges_info};
}

const RoutesGraph::EdgeInfo* RoutesGraph::GetEdgeInfo(EdgeId edge_id) const {
    return &edges_index_.at(edge_id);
}

double RoutesGraph::GetEdgeWeight(EdgeId edge_id) const {
    if (!routes_graph_) {
        throw std::logic_error("Graph doesn't exist yet."s);
    }
    return routes_graph_->GetEdge(edge_id).weight;
}

void RoutesGraph::AddVertexes(const RouteSettings& settings) {
    const auto& stops_list = db_.GetStopsList();
    size_t vertex_id = 0;
    for (const transport::Stop& stop : stops_list) {
        size_t edge_id = routes_graph_->AddEdge({vertex_id, vertex_id + 1,
            static_cast<double>(settings.bus_wait_time)});
        vertex_index_[&stop] = {vertex_id, vertex_id + 1};
        edges_index_[edge_id] = {&stop, &stop, nullptr, 0, static_cast<double>(settings.bus_wait_time)};
        vertex_id += 2;
    }
}

void RoutesGraph::AddRouteEdges(const RouteSettings& settings) {
    const auto& buses_list = db_.GetRoutesList();
    for (const transport::Bus& bus : buses_list) {
        int stop_index = 1;
        for (const transport::Stop* from : bus.route) {
            int span_count = 1;
            double weight_direct = 0.;
            double weight_opposite = 0.;
            const transport::Stop* cur_stop = from;
            for (auto iter_to = bus.route.begin() + stop_index; iter_to != bus.route.end(); ++iter_to) {
                const transport::Stop* to = *iter_to;
                weight_direct += 1. * db_.GetDistance(cur_stop, to) / (settings.bus_velocity * KPH_TO_MPS_SPEED_COEF);

                size_t edge_id = routes_graph_->AddEdge({GetStopVertexes(from).second,
                    GetStopVertexes(to).first, weight_direct});
                edges_index_[edge_id] = {from, to, &bus, span_count, weight_direct};
                if (!bus.is_round) {
                    weight_opposite += 1. * db_.GetDistance(to, cur_stop) / (settings.bus_velocity * KPH_TO_MPS_SPEED_COEF);
                    edge_id = routes_graph_->AddEdge({GetStopVertexes(to).second,
                        GetStopVertexes(from).first, weight_opposite});
                    edges_index_[edge_id] = {to, from, &bus, span_count, weight_opposite};
                }
                cur_stop = to;
                ++span_count;
            }
            ++stop_index;
        }
    }
}

void RoutesGraph::BuildGraph(const RouteSettings& settings) {
    routes_graph_ = std::make_unique<DirectedWeightedGraph<double>>(db_.GetStopsList().size() * 2);
    AddVertexes(settings);
    AddRouteEdges(settings);
}

void RoutesGraph::BuildRouter() {
    if (!routes_graph_) {
        throw std::logic_error("Graph doesn't exist yet."s);
    }
    router_ = std::make_unique<Router<double>>(*routes_graph_);
}
} // namespace graph
