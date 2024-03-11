#include "request_handler.h"

namespace transport {

namespace handler {

RequestHandler::RequestHandler(const TransportCatalogue& db, map_renderer::MapRenderer& renderer)
    : db_(db)
    , renderer_(renderer) {
}

const Bus* RequestHandler::GetBusStat(const std::string_view& bus_name) const {
    return db_.GetBusInfo(bus_name);
}

// Возвращает маршруты, проходящие через
const std::set<std::string_view>* RequestHandler::GetBusesByStop(const std::string_view& stop_name) const {
    return db_.GetBusesListForStop(stop_name);
}

void RequestHandler::RenderMap(std::ostream& output) {
    renderer_.AddMapObjects(db_.GetRoutesList());
    renderer_.DrawMap(output);
}

} // namespace transport::infrastructure
} // namespace infrastructure
