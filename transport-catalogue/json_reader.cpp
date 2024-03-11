#include "json_reader.h"

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
    json::Dict all_requests = json::Load(input).GetRoot().AsMap();

    if (all_requests.size() > 3) {
        throw std::logic_error("Unknown JSON document."s);
    }

    return {json::Document{all_requests.at("base_requests"s).AsArray()},
            json::Document{all_requests.at("stat_requests"s).AsArray()},
            json::Document{all_requests.at("render_settings"s).AsMap()}};
}

} // namespace transport::json_reader::detail

JsonReader::JsonReader(std::istream& input, TransportCatalogue& catalogue, handler::RequestHandler& handler)
    : requests_(detail::ReadJson(input))
    , catalogue_(catalogue)
    , handler_(handler) {
}

const json::Document& JsonReader::TakeRenderSettings() const{
    return requests_.render_settings;
}

TransportCatalogue& JsonReader::BuildCatalogue() {
    LoadStops();
    LoadBuses();
    return catalogue_;
}

void JsonReader::PrintStat(std::ostream& output) {
    Print(ProcessRequests(), output);
}

void JsonReader::LoadStops() {
    std::map<std::string_view, const json::Node*> road_distances;

    for (const auto& node : requests_.base_requests.GetRoot().AsArray()) {
        const json::Dict& data = node.AsMap();
        if (data.at("type"s).AsString() == "Stop"sv) {
            std::string_view stop_name = data.at("name"s).AsString();
            catalogue_.AddStop({std::string(stop_name),
                {data.at("latitude"s).AsDouble(), data.at("longitude"s).AsDouble()}});
                road_distances[stop_name] = &data.at("road_distances"s);
        }
    }

    for (auto& [from, node] : road_distances) {
        const json::Dict& distance_info = node->AsMap();
        for (const auto& [to, length_node] : distance_info) {
            const Stop* stop_from = catalogue_.GetStopInfo(from);
            const Stop* stop_to = catalogue_.GetStopInfo(to);
            catalogue_.SetDistance(stop_from, stop_to, length_node.AsInt());
        }
    }
}

void JsonReader::LoadBuses() {
    for (const auto& node : requests_.base_requests.GetRoot().AsArray()) {
        const json::Dict& data = node.AsMap();
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
    json::Dict stop_info;
    stop_info.emplace("request_id"s, json::Node{request_info.at("id"s).AsInt()});

    const std::set<std::string_view>* bus_list = handler_.GetBusesByStop(request_info.at("name"s).AsString());
    if (bus_list == nullptr) {
        stop_info.emplace("error_message"s, json::Node("not found"s));
        return json::Node{stop_info};
    }
    json::Array bus_list_as_array;
    for (const std::string_view bus : *bus_list) {
        bus_list_as_array.push_back(json::Node(std::string(bus)));
    }

    stop_info.emplace("buses"s, json::Node(bus_list_as_array));

    return json::Node{stop_info};
}

json::Node JsonReader::ProcessBusRequest(const json::Dict& request_info) {
    json::Dict bus_info;
    bus_info.emplace("request_id"s, json::Node{request_info.at("id"s).AsInt()});

    const Bus* bus_stat = handler_.GetBusStat(request_info.at("name"s).AsString());
    if (bus_stat == nullptr) {
        bus_info.emplace("error_message"s, json::Node{"not found"s});
        return json::Node{bus_info};
    }
    bus_info.emplace("stop_count"s, json::Node{bus_stat->GetStopsAmount()});
    bus_info.emplace("route_length"s, json::Node{bus_stat->geo_length});
    bus_info.emplace("curvature"s, json::Node{bus_stat->ComputeCurvature()});
    bus_info.emplace("unique_stop_count"s, json::Node{static_cast<int>(bus_stat->unique_stops_amount)});

    return json::Node{bus_info};
}

json::Node JsonReader::ProcessMapRequest(const json::Dict& request_info) {
    json::Dict map;
    map.emplace("request_id"s, json::Node{request_info.at("id"s).AsInt()});

    std::ostringstream svg;
    handler_.RenderMap(svg);
    map.emplace("map"s, json::Node{svg.str()});
    return json::Node{map};
}

json::Document JsonReader::ProcessRequests() {
    json::Array answers;
    for (const json::Node& node_request : requests_.stat_requests.GetRoot().AsArray()) {
        const json::Dict& request_info = node_request.AsMap();
        if (request_info.at("type"s).AsString() == "Stop"sv) {
            answers.push_back(ProcessStopRequest(request_info));
        } else if (request_info.at("type"s).AsString() == "Bus"sv) {
            answers.push_back(ProcessBusRequest(request_info));
        } else if (request_info.at("type"s).AsString() == "Map"sv) {
            answers.push_back(ProcessMapRequest(request_info));
        }
    }

    return json::Document(json::Node(answers));
}

} // namespace transport::json_reader
} // namespace transport
