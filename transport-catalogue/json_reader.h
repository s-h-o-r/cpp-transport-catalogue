#pragma once

#include "json.h"
#include "json_builder.h"
#include "request_handler.h"
#include "transport_catalogue.h"

#include <iostream>

namespace transport {

namespace json_reader {

struct Requests {
    json::Document base_requests;
    json::Document stat_requests;
    json::Document render_settings;
    json::Document routing_settings;
};

class JsonReader {
public:

    JsonReader(std::istream& input, TransportCatalogue& catalogue, 
               handler::RequestHandler& handler);

    const json::Document& TakeRenderSettings() const;
    const json::Document& TakeRoutingSettings() const;

    TransportCatalogue& BuildCatalogue();

    void PrintStat(std::ostream& output);

private:
    Requests requests_;
    TransportCatalogue& catalogue_;
    RoutesGraph routes_graph_;
    handler::RequestHandler& handler_;

    void LoadStops();
    void LoadBuses();

    json::Node ProcessStopRequest(const json::Dict& request_info);
    json::Node ProcessBusRequest(const json::Dict& request_info);
    json::Node ProcessMapRequest(const json::Dict& request_info);
    json::Node ProcessRouteRequest(const json::Dict& request_info);
    json::Document ProcessRequests();
};

} // namespace transport::json_reader
} // namespace transport

