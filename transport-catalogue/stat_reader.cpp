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

void PrintStat(const TransportCatalogue& catalogue, const Request& request,
               std::ostream& output) {
    using namespace std::string_literals;
    if (request.name == "Bus"s) {
        output << "Bus "s << request.id << ": "s;
        if (!catalogue.IsRouteExisted(request.id)) {
            output  << "not found\n"s;
        } else {
            std::string_view bus_name = request.id;
            output << catalogue.GetRouteStopsAmount(bus_name) << " stops on route, "s
            << catalogue.GetRouteUniqueStopsAmount(bus_name) << " unique stops, "s
            << std::setprecision(6) << catalogue.GetRouteLenght(bus_name) << " route length\n"s;
        }
    } else if (request.name == "Stop"s) {
        output << "Stop "s << request.id << ": "s;
        if (!catalogue.IsStopExisted(request.id)) {
            output  << "not found\n"s;
        } else {
            std::string_view stop_name = request.id;
            const std::set<std::string_view> buses_on_stop = catalogue.GetBusesListForStop(stop_name);
            if (buses_on_stop.empty()) {
                output << "no buses\n"s;
            } else {
                output << "buses ";
                for (const auto bus : buses_on_stop) {
                    output << bus << " "s;
                }
                output << "\n"s;
            }
        }
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
