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

void PrintStat(const TransportCatalogue& tansport_catalogue,
               const Request& request, std::ostream& output);

} // namespace transport::stat::detail

void ParseAndPrint(const TransportCatalogue& tansport_catalogue,
                   std::string_view request, std::ostream& output);


} // namespace transport::stat
} // namespace transport
