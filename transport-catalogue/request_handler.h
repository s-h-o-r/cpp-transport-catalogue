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
    // MapRenderer понадобится в следующей части итогового проекта
//    RequestHandler(const TransportCatalogue& db, const renderer::MapRenderer& renderer);
    RequestHandler(const TransportCatalogue& db, map_renderer::MapRenderer& renderer);

    // Возвращает информацию о маршруте (запрос Bus)
    const Bus* GetBusStat(const std::string_view& bus_name) const;

    // Возвращает маршруты, проходящие через
    const std::set<std::string_view>* GetBusesByStop(const std::string_view& stop_name) const;

    void RenderMap(std::ostream& output);

private:
    // RequestHandler использует агрегацию объектов "Транспортный Справочник" и "Визуализатор Карты"
    const TransportCatalogue& db_;
    map_renderer::MapRenderer& renderer_;
};



} // namespace transport::infrastructure
} // namespace infrastructure
