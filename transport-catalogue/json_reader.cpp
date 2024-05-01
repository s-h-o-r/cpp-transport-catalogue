#include "json_reader.h"
#include "router.h"

#include <algorithm>
#include <map>
#include <set>
#include <stdexcept>
#include <sstream>
#include <string_view>
#include <string>
#include <vector>

namespace transport {
namespace json_reader {

using namespace std::literals;

namespace detail {

Requests ReadJson(std::istream& input) {
    json::Dict all_requests = json::Load(input).GetRoot().AsDict();

    if (all_requests.size() > 4) {
        throw std::logic_error("Unknown JSON document."s);
    }

    return {json::Document{json::Builder{}.Value(all_requests.at("base_requests"s).AsArray()).Build()},
        json::Document{json::Builder{}.Value(all_requests.at("stat_requests"s).AsArray()).Build()},
        json::Document{json::Builder{}.Value(all_requests.at("render_settings"s).AsDict()).Build()},
        json::Document{json::Builder{}.Value(all_requests.at("routing_settings"s).AsDict()).Build()}};
}

} // namespace transport::json_reader::detail

JsonReader::JsonReader(std::istream& input, TransportCatalogue& catalogue, handler::RequestHandler& handler)
    : requests_(detail::ReadJson(input))
    , catalogue_(catalogue)
    , routes_graph_(catalogue_)
    , handler_(handler) {
}

const json::Document& JsonReader::TakeRenderSettings() const {
    return requests_.render_settings;
}

const json::Document& JsonReader::TakeRoutingSettings() const {
    return requests_.routing_settings;
}

TransportCatalogue& JsonReader::BuildCatalogue() {
    LoadStops();
    LoadBuses();

    auto settings = requests_.routing_settings.GetRoot().AsDict();
    RouteSettings route_settings{settings.at("bus_wait_time"s).AsInt(), settings.at("bus_velocity"s).AsInt()};
    routes_graph_.BuildGraph(route_settings);

    return catalogue_;
}

void JsonReader::PrintStat(std::ostream& output) {
    Print(ProcessRequests(), output);
}

void JsonReader::LoadStops() {
    std::map<std::string_view, const json::Node*> road_distances;

    for (const auto& node : requests_.base_requests.GetRoot().AsArray()) {
        const json::Dict& data = node.AsDict();
        if (data.at("type"s).AsString() == "Stop"sv) {
            std::string_view stop_name = data.at("name"s).AsString();
            catalogue_.AddStop({std::string(stop_name),
                {data.at("latitude"s).AsDouble(), data.at("longitude"s).AsDouble()}});
                road_distances[stop_name] = &data.at("road_distances"s);
        }
    }

    for (auto& [from, node] : road_distances) {
        const json::Dict& distance_info = node->AsDict();
        for (const auto& [to, length_node] : distance_info) {
            const Stop* stop_from = catalogue_.GetStopInfo(from);
            const Stop* stop_to = catalogue_.GetStopInfo(to);
            catalogue_.SetDistance(stop_from, stop_to, length_node.AsInt());
        }
    }
}

void JsonReader::LoadBuses() {
    for (const auto& node : requests_.base_requests.GetRoot().AsArray()) {
        const json::Dict& data = node.AsDict();
        if (data.at("type").AsString() == "Bus"sv) {
            std::vector<std::string_view> route;
            for (const auto& node : data.at("stops"s).AsArray()) {
                route.push_back(node.AsString());
            }

            catalogue_.AddBus(data.at("name"s).AsString(), route, data.at("is_roundtrip").AsBool());
        }
    }
}

json::Node JsonReader::ProcessStopRequest(const json::Dict& request_info) {
    json::Builder stop_info;

    stop_info.StartDict()
                .Key("request_id"s).Value(request_info.at("id"s).AsInt());

    const std::set<std::string_view>* bus_list = handler_.GetBusesByStop(request_info.at("name"s).AsString());
    if (bus_list == nullptr) {
        return stop_info.Key("error_message"s).Value("not found"s)
                .EndDict()
                .Build();
    }

    stop_info.Key("buses"s).StartArray();
    for (const std::string_view bus : *bus_list) {
        stop_info.Value(std::string(bus));
    }

    return stop_info.EndArray().EndDict().Build();
}

json::Node JsonReader::ProcessBusRequest(const json::Dict& request_info) {
    json::Builder bus_info;
    bus_info.StartDict()
               .Key("request_id"s).Value(request_info.at("id"s).AsInt());

    const Bus* bus_stat = handler_.GetBusStat(request_info.at("name"s).AsString());
    if (bus_stat == nullptr) {
        return bus_info.Key("error_message"s).Value("not found"s)
                .EndDict()
                .Build();
    }

    bus_info.Key("stop_count"s).Value(bus_stat->GetStopsAmount())
           .Key("route_length"s).Value(bus_stat->geo_length)
           .Key("curvature"s).Value(bus_stat->ComputeCurvature())
           .Key("unique_stop_count"s).Value(static_cast<int>(bus_stat->unique_stops_amount));

    return bus_info.EndDict()
                  .Build();
}

json::Node JsonReader::ProcessMapRequest(const json::Dict& request_info) {
    std::ostringstream svg;
    handler_.RenderMap(svg);
    return json::Builder{}.StartDict()
                              .Key("request_id"s).Value(request_info.at("id"s).AsInt())
                              .Key("map"s).Value(svg.str())
                          .EndDict()
                          .Build();
}

json::Node JsonReader::ProcessRouteRequest(const json::Dict& request_info) {
    json::Builder answer;
    answer.StartDict()
            .Key("request_id"s).Value(request_info.at("id"s).AsInt());

    if (catalogue_.GetStopInfo(request_info.at("from"s).AsString()) == nullptr
        || catalogue_.GetStopInfo(request_info.at("to"s).AsString()) == nullptr) {
        return answer.Key("error_message"s).Value("not found"s)
                    .EndDict()
                    .Build();
    }

    auto route_info = routes_graph_.BuildRoute(catalogue_.GetStopInfo(request_info.at("from"s).AsString()),
                                               catalogue_.GetStopInfo(request_info.at("to"s).AsString()));

    if (!route_info) {
        answer.Key("error_message"s).Value("not found"s);
    } else {
        answer.Key("total_time"s).Value(route_info.value().weight)
              .Key("items"s).StartArray();
        
        for (graph::EdgeId edge_id : route_info.value().edges) {
            const EdgeInfo* edge_info = routes_graph_.GetEdgeInfo(edge_id);
            if (edge_info->from == edge_info->to) {
                answer.StartDict()
                        .Key("type"s).Value("Wait"s)
                        .Key("stop_name"s).Value(edge_info->from->name)
                        .Key("time").Value(routes_graph_.GetEdgeWeight(edge_id))
                      .EndDict();
            } else {
                answer.StartDict()
                        .Key("type"s).Value("Bus"s)
                        .Key("bus"s).Value(edge_info->bus->name)
                        .Key("span_count"s).Value(edge_info->span_count)
                        .Key("time").Value(routes_graph_.GetEdgeWeight(edge_id))
                      .EndDict();
            }
        }
        answer.EndArray();
    }

    return answer.EndDict().Build();

}

json::Document JsonReader::ProcessRequests() {
    json::Builder answers;
    answers.StartArray();
    for (const json::Node& node_request : requests_.stat_requests.GetRoot().AsArray()) {
        const json::Dict& request_info = node_request.AsDict();
        if (request_info.at("type"s).AsString() == "Stop"sv) {
            answers.Value(ProcessStopRequest(request_info));
        } else if (request_info.at("type"s).AsString() == "Bus"sv) {
            answers.Value(ProcessBusRequest(request_info));
        } else if (request_info.at("type"s).AsString() == "Map"sv) {
            answers.Value(ProcessMapRequest(request_info));
        } else if (request_info.at("type"s).AsString() == "Route"sv) {
            answers.Value(ProcessRouteRequest(request_info));
        }
    }

    return json::Document{answers.EndArray().Build()};
}

} // namespace transport::json_reader
} // namespace transport
