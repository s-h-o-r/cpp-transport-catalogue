#include "json_reader.h"
#include "transport_catalogue.h"
#include "request_handler.h"
#include "map_renderer.h"

#include <iostream>

using namespace transport;
using namespace std::literals;

int main() {
    TransportCatalogue catalogue;
    map_renderer::MapRenderer renderer;
    handler::RequestHandler request_handler(catalogue, renderer);

    json_reader::JsonReader json_reader(std::cin, catalogue, request_handler);
    json_reader.BuildCatalogue();

    renderer.SetSettings(json_reader.TakeRenderSettings());
    json_reader.PrintStat(std::cout);
}
