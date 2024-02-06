#include "stat_reader.h"

#include <iomanip>
#include <iostream>

namespace transport {
namespace stat {
namespace detail {

Request ParseRequest(std::string_view request) {
    auto first_symbol = request.find_first_not_of(' ');
    auto last_symbol = request.find_last_not_of(' ');
    auto space_between = request.find_first_of(' ', first_symbol);
    auto first_symbol2 = request.find_first_not_of(' ', space_between);

    return { std::string(request.substr(first_symbol, space_between - first_symbol)),
        std::string(request.substr(first_symbol2, last_symbol - first_symbol2 + 1)) };
}

void PrintBusStat(const TransportCatalogue& catalogue, std::string_view bus_name,
                  std::ostream& output) {
    using namespace std::string_literals;
    output << "Bus "s << bus_name << ": "s;
    const Bus* bus_info = catalogue.IsRouteExisted(bus_name);
    if (bus_info == nullptr) {
        output  << "not found\n"s;
    } else {
        auto [length, curvature] = catalogue.GetGeoLengthAndCurvature(bus_info);

        output << bus_info->route.size() << " stops on route, "s
        << bus_info->unique_stops_amount << " unique stops, "s
        << length << " route length, "s
        << std::setprecision(6) << curvature << " curvature\n";
    }
}

void PrintStopStat(const TransportCatalogue& catalogue, std::string_view stop_name,
                   std::ostream& output) {
    using namespace std::string_literals;
    output << "Stop "s << stop_name << ": "s;
    if (catalogue.IsStopExisted(stop_name) == nullptr) {
        output  << "not found\n"s;
    } else {
        const std::set<std::string_view> buses_on_stop = catalogue.GetBusesListForStop(stop_name);
        if (buses_on_stop.empty()) {
            output << "no buses\n"s;
        } else {
            output << "buses";
            for (const auto bus : buses_on_stop) {
                output << " "s << bus;
            }
            output << "\n"s;
        }
    }
}

void PrintStat(const TransportCatalogue& catalogue, const Request& request,
               std::ostream& output) {
    using namespace std::string_literals;
    if (request.name == "Bus"s) {
        PrintBusStat(catalogue, request.id, output);
    } else if (request.name == "Stop"s) {
        PrintStopStat(catalogue, request.id, output);
    }
}

} // namespace transport::detail

void ParseAndPrint(const TransportCatalogue& tansport_catalogue,
                       std::string_view request, std::ostream& output) {
    if (!request.empty()) {
        detail::PrintStat(tansport_catalogue, detail::ParseRequest(request), output);
    }
}
} // namespace transport::stat
} // namespace transport
