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

const std::deque<Bus>& TransportCatalogue::GetRoutesList() const {
    return buses_;
}

const std::deque<Stop>& TransportCatalogue::GetStopsList() const {
    return stops_;
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

RoutesGraph::RoutesGraph(const TransportCatalogue& db)
    : db_(db) {
}

std::pair<size_t, size_t> RoutesGraph::GetStopVertexes(const Stop* stop) const {
    return vertex_index_.at(stop);
}

const graph::DirectedWeightedGraph<double>* RoutesGraph::GetGraph() const {
    if (!routes_graph_.has_value()) {
        return nullptr;
    }
    return &routes_graph_.value();
}

const EdgeInfo* RoutesGraph::GetEdgeInfo(graph::EdgeId edge_id) const {
    return &edges_index_.at(edge_id);
}

double RoutesGraph::GetEdgeWeight(graph::EdgeId edge_id) const {
    assert(routes_graph_.has_value());
    return routes_graph_->GetEdge(edge_id).weight;
}

void RoutesGraph::BuildGraph(const RouteSettings& settings) {
    AddVertexes(settings);
    AddRouteEdges(settings);
    BuildRouter();
}

std::optional<graph::Router<double>::RouteInfo>
RoutesGraph::BuildRoute(const Stop* from, const Stop* to) const {
    assert(router_.has_value());
    size_t vertex_from = GetStopVertexes(from).first;
    size_t vertex_to = GetStopVertexes(to).first;
    return router_->BuildRoute(vertex_from, vertex_to);
}

void RoutesGraph::AddVertexes(const RouteSettings& settings) {
    const auto& stops_list = db_.GetStopsList();
    routes_graph_ = graph::DirectedWeightedGraph<double>(stops_list.size() * 2);
    size_t vertex_id = 0;
    for (const Stop& stop : stops_list) {
        size_t edge_id = routes_graph_->AddEdge({vertex_id, vertex_id + 1,
                                        static_cast<double>(settings.bus_wait_time)});
        vertex_index_[&stop] = {vertex_id, vertex_id + 1};
        edges_index_[edge_id] = {&stop, &stop, nullptr, 0, 6.};
        vertex_id += 2;
    }
}

void RoutesGraph::AddRouteEdges(const RouteSettings& settings) {
    const auto& buses_list = db_.GetRoutesList();
    for (const Bus& bus : buses_list) {
        int stop_index = 1;
        for (const Stop* from : bus.route) {
            int span_count = 1;
            double weight_direct = 0.;
            double weight_opposite = 0.;
            const Stop* cur_stop = from;
            for (auto iter_to = bus.route.begin() + stop_index; iter_to != bus.route.end(); ++iter_to) {
                const Stop* to = *iter_to;
                weight_direct += 1. * db_.GetDistance(cur_stop, to) / (settings.bus_velocity * 1000. / 60.);

                size_t edge_id = routes_graph_->AddEdge({GetStopVertexes(from).second,
                    GetStopVertexes(to).first, weight_direct});
                edges_index_[edge_id] = {from, to, &bus, span_count, weight_direct};
                if (!bus.is_round) {
                    weight_opposite += 1. * db_.GetDistance(to, cur_stop) / (settings.bus_velocity * 1000. / 60.);
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

void RoutesGraph::BuildRouter() {
    assert(routes_graph_.has_value());
    router_.emplace(graph::Router<double>{*routes_graph_});
}
} // namespace transport

