#include "input_reader.h"

#include <algorithm>
#include <cassert>
#include <iterator>

namespace transport {
namespace input {
namespace detail {

Coordinates ParseCoordinates(std::string_view str) {
    static const double nan = std::nan("");

    auto not_space = str.find_first_not_of(' ');
    auto comma = str.find(',');

    if (comma == str.npos) {
        return {nan, nan};
    }

    auto not_space2 = str.find_first_not_of(' ', comma + 1);

    double lat = std::stod(std::string(str.substr(not_space, comma - not_space)));
    double lng = std::stod(std::string(str.substr(not_space2)));

    return {lat, lng};
}

/**
 * Удаляет пробелы в начале и конце строки
 */
std::string_view Trim(std::string_view string) {
    const auto start = string.find_first_not_of(' ');
    if (start == string.npos) {
        return {};
    }
    return string.substr(start, string.find_last_not_of(' ') + 1 - start);
}

/**
 * Разбивает строку string на n строк, с помощью указанного символа-разделителя delim
 */
std::vector<std::string_view> Split(std::string_view string, char delim) {
    std::vector<std::string_view> result;

    size_t pos = 0;
    while ((pos = string.find_first_not_of(' ', pos)) < string.length()) {
        auto delim_pos = string.find(delim, pos);
        if (delim_pos == string.npos) {
            delim_pos = string.size();
        }
        if (auto substr = Trim(string.substr(pos, delim_pos - pos)); !substr.empty()) {
            result.push_back(substr);
        }
        pos = delim_pos + 1;
    }

    return result;
}

/**
 * Парсит маршрут.
 * Для кольцевого маршрута (A>B>C>A) возвращает массив названий остановок [A,B,C,A]
 * Для некольцевого маршрута (A-B-C-D) возвращает массив названий остановок [A,B,C,D,C,B,A]
 */
std::vector<std::string_view> ParseRoute(std::string_view route) {
    if (route.find('>') != route.npos) {
        return Split(route, '>');
    }

    auto stops = Split(route, '-');
    std::vector<std::string_view> results(stops.begin(), stops.end());
    results.insert(results.end(), std::next(stops.rbegin()), stops.rend());

    return results;
}

std::vector<DistanceTo> ParseDistances(std::string_view description) {
    std::vector<std::string_view> parsed_decription = Split(description, ',');
    parsed_decription.erase(parsed_decription.begin(), parsed_decription.begin() + 2); // удаляем координаты из вектора, которые тоже туда попали
    std::vector<DistanceTo> distances;
    for (const auto& string : parsed_decription) {
        auto m_pos = string.find('m');
        auto name_begin = m_pos + 5;
        distances.push_back({string.substr(name_begin, string.size() - name_begin), 
                             std::stoi(std::string(string.substr(0, m_pos)))});
    }
    return distances;
}

CommandDescription ParseCommandDescription(std::string_view line) {
    auto colon_pos = line.find(':');
    if (colon_pos == line.npos) {
        return {};
    }

    auto space_pos = line.find(' ');
    if (space_pos >= colon_pos) {
        return {};
    }

    auto not_space = line.find_first_not_of(' ', space_pos);
    if (not_space >= colon_pos) {
        return {};
    }

    return {std::string(line.substr(0, space_pos)),
        std::string(line.substr(not_space, colon_pos - not_space)),
        std::string(line.substr(colon_pos + 1))};
}

} // namespace transport::input::detail

void InputReader::ParseLine(std::string_view line) {
    using namespace std::string_literals;
    auto command_description = detail::ParseCommandDescription(line);
    if (command_description.command == "Stop"s) {
        stop_commands_.push_back(std::move(command_description));
    } else if (command_description.command == "Bus"s) {
        bus_commands_.push_back(std::move(command_description));
    }
}

void InputReader::ApplyCommands([[maybe_unused]] TransportCatalogue& catalogue) {
    ProcessStopCommands(catalogue);
    ProcessBusCommands(catalogue);
}

void InputReader::ProcessStopCommands([[maybe_unused]] TransportCatalogue& catalogue) {
    for (auto& command : stop_commands_) {
        if (command) {
            catalogue.AddStop(command.id, detail::ParseCoordinates(command.description));
        }
    }

    for (auto& command : stop_commands_) {
        if (command) {
            for (auto& [next_stop_name, length_to_stop] : detail::ParseDistances(command.description)) {
                const Stop* stop_from = catalogue.GetStopInfo(command.id);
                const Stop* stop_to = catalogue.GetStopInfo(next_stop_name);
                catalogue.SetDistance(stop_from, stop_to, length_to_stop);
            }
        }
    }
}

void InputReader::ProcessBusCommands([[maybe_unused]] TransportCatalogue& catalogue) {
    for (auto& command : bus_commands_) {
        if (command) {
            catalogue.AddBus(command.id, detail::ParseRoute(command.description));
        }
    }
}

} // namespace transport::input
} // namespace transport
