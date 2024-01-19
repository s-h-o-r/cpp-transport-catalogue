#pragma once

#include <iosfwd>
#include <string>
#include <string_view>

#include "transport_catalogue.h"

namespace transport {
namespace stat {
namespace detail {

struct Request {
    std::string name;
    std::string id;
};

Request ParseRequest(std::string_view request);

void PrintBusStat(const TransportCatalogue& catalogue, std::string_view bus_name,
                  std::ostream& output);

void PrintStopStat(const TransportCatalogue& catalogue, std::string_view stop_name,
                   std::ostream& output);

void PrintStat(const TransportCatalogue& tansport_catalogue,
               const Request& request, std::ostream& output);

} // namespace transport::stat::detail

void ParseAndPrint(const TransportCatalogue& tansport_catalogue,
                   std::string_view request, std::ostream& output);


} // namespace transport::stat
} // namespace transport
