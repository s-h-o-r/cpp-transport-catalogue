#include "map_renderer.h"

#include <cmath>
#include <map>

namespace map_renderer {

using namespace svg;

bool IsZero(double value) {
    return std::abs(value) < EPSILON;
}

void MapRenderer::SetSettings(const json::Document& render_settings) {
    for (const auto& [setting_name, node] : render_settings.GetRoot().AsDict()) {
        if (setting_name == "width"sv) {
            render_settings_.width = node.AsDouble();
        } else if (setting_name == "height"sv) {
            render_settings_.height = node.AsDouble();
        } else if (setting_name == "padding"sv) {
            render_settings_.padding = node.AsDouble();
        } else if (setting_name == "line_width"sv) {
            render_settings_.line_width = node.AsDouble();
        } else if (setting_name == "stop_radius"sv) {
            render_settings_.stop_radius = node.AsDouble();
        } else if (setting_name == "bus_label_font_size"sv) {
            render_settings_.bus_label_font_size = node.AsInt();
        } else if (setting_name == "bus_label_offset"sv) {
            const json::Array& values = node.AsArray();
            render_settings_.bus_label_offset = {values[0].AsDouble(), values[1].AsDouble()};
        } else if (setting_name == "stop_label_font_size"sv) {
            render_settings_.stop_label_font_size = node.AsInt();
        } else if (setting_name == "stop_label_offset"sv) {
            const json::Array& values = node.AsArray();
            render_settings_.stop_label_offset = {values[0].AsDouble(), values[1].AsDouble()};
        } else if (setting_name == "underlayer_color"sv) {
            render_settings_.underlayer_color = ProcessColorSetting(node);
        } else if (setting_name == "underlayer_width"sv) {
            render_settings_.underlayer_width = node.AsDouble();
        } else if (setting_name == "color_palette"sv) {
            for (const json::Node& color_node : node.AsArray()) {
                render_settings_.color_palette.push_back(ProcessColorSetting(color_node));
            }
        } else {
            throw std::logic_error("Unknown render setting."s);
        }
    }
}

void MapRenderer::RenderMapObjects(const std::deque<transport::Bus>& buses) {
    std::vector<const transport::Bus*> routes;
    for (const auto& bus : buses) {
        routes.push_back(&bus);
    }

    if (has_objects_to_draw_) {
        return;
    }
    std::sort(routes.begin(), routes.end(), [](const transport::Bus* lhs,
                                               const transport::Bus* rhs) {return lhs->name < rhs->name; });

    std::vector<geo::Coordinates> all_geo_coords;
    std::vector<const transport::Stop*> all_stops;

    for (const transport::Bus* bus : routes) {
        for (const transport::Stop* stop : bus->route) {
            if (std::find(all_stops.begin(), all_stops.end(), stop) == all_stops.end()) {
                all_geo_coords.push_back(stop->coordinates);
                all_stops.push_back(stop);
            }
        }
    }

    const SphereProjector proj{
        all_geo_coords.begin(), all_geo_coords.end(),
        render_settings_.width, render_settings_.height, render_settings_.padding
    };

    RenderRoutes(routes, proj);
    RenderRoutesNames(routes, proj);

    std::sort(all_stops.begin(), all_stops.end(), [](const transport::Stop* lhs,
                                               const transport::Stop* rhs) {return lhs->name < rhs->name; });
    RenderStopsSymbols(all_stops, proj);
    RenderStopsNames(all_stops, proj);
    has_objects_to_draw_ = true;
}

void MapRenderer::DrawMap(std::ostream& output) {
    objects_to_draw_.Render(output);
}

void MapRenderer::RenderRoutes(std::vector<const transport::Bus*>& routes,
                           const SphereProjector& proj) {

    size_t color_number = 0;
    for (const transport::Bus* bus : routes) {
        if (bus->route.empty()) {
            continue;
        }
        std::vector<geo::Coordinates> geo_coords;
        for (const transport::Stop* stop : bus->route) {
            geo_coords.push_back(stop->coordinates);
        }
        if (!bus->is_round) {
            for (auto ptr = bus->route.rbegin() + 1; ptr != bus->route.rend(); ++ptr) {
                const transport::Stop* stop = *ptr;
                geo_coords.push_back(stop->coordinates);
            }
        }

        svg::Polyline svg_route;
        for (const auto& geo_coord : geo_coords) {
            svg_route.AddPoint(proj(geo_coord));
        }

        objects_to_draw_.Add(svg_route
                             .SetFillColor(render_settings_.fill_color)
                             .SetStrokeColor(render_settings_.color_palette[color_number])
                             .SetStrokeWidth(render_settings_.line_width)
                             .SetStrokeLineCap(render_settings_.stroke_linecap)
                             .SetStrokeLineJoin(render_settings_.stroke_linejoin));

        if (color_number < render_settings_.color_palette.size() - 1) {
            ++color_number;
        } else {
            color_number = 0;
        }
    }
}

void MapRenderer::RenderRoutesNames(std::vector<const transport::Bus*>& routes,
                    const SphereProjector& proj) {
    size_t color_number = 0;
    for (const transport::Bus* bus : routes) {
        if (bus->route.empty()) {
            continue;
        }
        // название маршрута, кольцевой или нет + координаты либо первой, либо первой и последней остановки

        svg::Text route_name1 = Text()
                    .SetData(bus->name)
                    .SetOffset(render_settings_.bus_label_offset)
                    .SetFontSize(render_settings_.bus_label_font_size)
                    .SetFontFamily("Verdana"s)
                    .SetFontWeight("bold"s)
                    .SetPosition(proj(bus->route[0]->coordinates))
                    .SetFillColor(render_settings_.color_palette[color_number]);

        svg::Text underlayer1 = route_name1;
        objects_to_draw_.Add(underlayer1.SetFillColor(render_settings_.underlayer_color)
                   .SetStrokeColor(render_settings_.underlayer_color)
                   .SetStrokeWidth(render_settings_.underlayer_width)
                   .SetStrokeLineCap(render_settings_.stroke_linecap)
                   .SetStrokeLineJoin(render_settings_.stroke_linejoin));

        objects_to_draw_.Add(route_name1);


        if (bus->route[0] != bus->route[bus->route.size() - 1]
            && !bus->is_round) {
            svg::Text route_name2 = route_name1;
            svg::Text underlayer2 = underlayer1;

            objects_to_draw_.Add(underlayer2.SetPosition(proj(bus->route[bus->route.size() - 1]->coordinates)));
            objects_to_draw_.Add(route_name2.SetPosition(proj(bus->route[bus->route.size() - 1]->coordinates)));
        }

        if (color_number < render_settings_.color_palette.size() - 1) {
            ++color_number;
        } else {
            color_number = 0;
        }
    }
}

void MapRenderer::RenderStopsSymbols(std::vector<const transport::Stop*> stops,
                                  const SphereProjector& proj) {
    for (const transport::Stop* stop : stops) {
        objects_to_draw_.Add(Circle()
                             .SetCenter(proj(stop->coordinates))
                             .SetRadius(render_settings_.stop_radius)
                             .SetFillColor("white"s));
    }
}

void MapRenderer::RenderStopsNames(std::vector<const transport::Stop*> stops,
                                const SphereProjector& proj) {
    for (const transport::Stop* stop : stops) {
        svg::Text stop_name = Text()
                    .SetPosition(proj(stop->coordinates))
                    .SetOffset(render_settings_.stop_label_offset)
                    .SetFontSize(render_settings_.stop_label_font_size)
                    .SetFontFamily("Verdana"s)
                    .SetFillColor("black"s)
                    .SetData(stop->name);

        svg::Text underlayer = stop_name;
        underlayer.SetFillColor(render_settings_.underlayer_color)
            .SetStrokeColor(render_settings_.underlayer_color)
            .SetStrokeWidth(render_settings_.underlayer_width)
            .SetStrokeLineCap(render_settings_.stroke_linecap)
            .SetStrokeLineJoin(render_settings_.stroke_linejoin);
        
        objects_to_draw_.Add(underlayer);
        objects_to_draw_.Add(stop_name);
    }
}

svg::Color MapRenderer::ProcessColorSetting(const json::Node color_node) {
    if (color_node.IsString()) {
        return color_node.AsString();
    } else if (color_node.IsArray()) {
        const json::Array& color_settings = color_node.AsArray();
        if (color_settings.size() == 4) {
            return svg::Rgba(color_settings[0].AsInt(),
                             color_settings[1].AsInt(),
                             color_settings[2].AsInt(),
                             color_settings[3].AsDouble());
        } else if (color_settings.size() == 3) {
            return svg::Rgb(color_settings[0].AsInt(),
                            color_settings[1].AsInt(),
                            color_settings[2].AsInt());
        } else {
            throw std::logic_error("Unknown format of color."s);
        }
    } else {
        throw std::logic_error("Unknown format of color."s);
    }
}

} // namespace map_renderer

