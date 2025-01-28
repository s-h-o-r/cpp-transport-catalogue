#pragma once

#include "transport_catalogue.h"
#include "map_renderer.h"

#include <iostream>
#include <set>
#include <string_view>
#include <unordered_set>

namespace transport {

namespace handler {

using namespace transport;

class RequestHandler {
public:
    RequestHandler(const TransportCatalogue& db, map_renderer::MapRenderer& renderer);

    const Bus* GetBusStat(const std::string_view& bus_name) const;
    const std::set<std::string_view>* GetBusesByStop(const std::string_view& stop_name) const;

    void RenderMap(std::ostream& output);

private:
    const TransportCatalogue& db_;
    map_renderer::MapRenderer& renderer_;
};



} // namespace transport::infrastructure
} // namespace infrastructure
