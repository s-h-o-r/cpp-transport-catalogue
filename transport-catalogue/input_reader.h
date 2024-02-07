#pragma once
#include <string>
#include <string_view>
#include <vector>

#include "geo.h"
#include "transport_catalogue.h"

namespace transport {
namespace input {

struct CommandDescription {
    // Определяет, задана ли команда (поле command непустое)
    explicit operator bool() const {
        return !command.empty();
    }

    bool operator!() const {
        return !operator bool();
    }

    std::string command;      // Название команды
    std::string id;           // id маршрута или остановки
    std::string description;  // Параметры команды
};

class InputReader {
public:

    void ParseLine(std::string_view line);

    void ApplyCommands(TransportCatalogue& catalogue);

private:
    std::vector<CommandDescription> bus_commands_;
    std::vector<CommandDescription> stop_commands_;

    void ProcessStopCommands([[maybe_unused]] TransportCatalogue& catalogue);
    void ProcessBusCommands([[maybe_unused]] TransportCatalogue& catalogue);
};
} // namespace transport::input
} // namespace transport
