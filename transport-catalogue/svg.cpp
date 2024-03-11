#include "svg.h"

#include <sstream>

namespace svg {

using namespace std::literals;

void EncodeText(std::ostream& out, std::string_view text) {

    for (char ch : text) {
        switch (ch) {
            case '"':
                out << "&quot;"sv;
                break;
            case '<':
                out << "&lt;"sv;
                break;
            case '>':
                out << "&gt;"sv;
                break;
            case '\'':
                out << "&apos;"sv;
                break;
            case '&':
                out << "&amp;"sv;
                break;
            default:
                out << ch;
                break;
        }
    }
}

std::string ColorGetter::operator()(std::monostate) const {
    std::ostringstream out;
    out << "none"s;
    return out.str();
}

std::string ColorGetter::operator()(const std::string color) const {
    return color;
}

std::string ColorGetter::operator()(Rgb rgb) const {
    std::ostringstream out;
    out << "rgb("s << std::to_string(rgb.red)
        << ","s << std::to_string(rgb.green)
        << ","s << std::to_string(rgb.blue) << ")"s;
    return out.str();
}

std::string ColorGetter::operator()(Rgba rgba) const {
    std::ostringstream out;
    out << "rgba("s << std::to_string(rgba.red)
        << ","s << std::to_string(rgba.green)
        << ","s << std::to_string(rgba.blue)
        << ","s << rgba.opacity << ")"s;
    return out.str();
}

std::ostream& operator<<(std::ostream& out, const Color& color) {
    out << std::visit(ColorGetter{}, color);
    return out;
}

std::ostream& operator<<(std::ostream& out, const StrokeLineCap& line_cap) {
    switch (line_cap) {
        case StrokeLineCap::BUTT:
            out << "butt"sv;
            break;
        case StrokeLineCap::ROUND:
            out << "round"sv;
            break;
        case StrokeLineCap::SQUARE:
            out << "square"sv;
            break;
    }
    return out;
}

std::ostream& operator<<(std::ostream& out, const StrokeLineJoin& line_join) {
    switch (line_join) {
        case StrokeLineJoin::ARCS:
            out << "arcs"sv;
            break;
        case StrokeLineJoin::BEVEL:
            out << "bevel"sv;
            break;
        case StrokeLineJoin::MITER:
            out << "miter"sv;
            break;
        case StrokeLineJoin::MITER_CLIP:
            out << "miter-clip"sv;
            break;
        case StrokeLineJoin::ROUND:
            out << "round"sv;
            break;
    }

    return out;
}

void Object::Render(const RenderContext& context) const {
    context.RenderIndent();

    // Делегируем вывод тега своим подклассам
    RenderObject(context);

    context.out << std::endl;
}

// ---------- Circle ------------------

Circle& Circle::SetCenter(Point center)  {
    center_ = center;
    return *this;
}

Circle& Circle::SetRadius(double radius)  {
    radius_ = radius;
    return *this;
}

void Circle::RenderObject(const RenderContext& context) const {
    auto& out = context.out;
    out << "<circle cx=\""sv << center_.x << "\" cy=\""sv << center_.y << "\""sv;
    out << " r=\""sv << radius_ << "\""sv;
    RenderAttrs(out);
    out << "/>"sv;
}

Polyline& Polyline::AddPoint(Point point) {
    points_.push_back(point);
    return *this;
}

void Polyline::RenderObject(const RenderContext& context) const {
    auto& out = context.out;
    out << "<polyline points=\""sv;
    bool need_space = false;
    for (const Point& point : points_) {
        if (need_space) {
            out << ' ';
        } else {
            need_space = true;
        }
        out << point.x << ","sv << point.y;
    }
    out << "\""sv;
    RenderAttrs(out);
    out << "/>"sv;
}

Text& Text::SetPosition(Point pos) {
    position_ = pos;
    return *this;
}

Text& Text::SetOffset(Point offset) {
    offset_ = offset;
    return *this;
}

Text& Text::SetFontSize(uint32_t size) {
    font_size_ = size;
    return *this;
}

Text& Text::SetFontFamily(std::string font_family) {
    font_family_ = font_family;
    return *this;
}

Text& Text::SetFontWeight(std::string font_weight) {
    font_weight_ = font_weight;
    return *this;
}

Text& Text::SetData(std::string data) {
    text_ = data;
    return *this;
}

void Text::RenderObject(const RenderContext& context) const {
    auto& out = context.out;
    out << "<text"sv;
    RenderAttrs(out);
    out << " x=\""sv << position_.x << "\" y=\""sv << position_.y << "\""sv;
    out << " dx=\""sv << offset_.x << "\" dy=\""sv << offset_.y << "\""sv;
    out << " font-size=\""sv << font_size_ << "\""sv;
    if (font_family_) {
        out << " font-family=\""sv << *font_family_ << "\""sv;
    }

    if (font_weight_) {
        out << " font-weight=\""sv << *font_weight_ << "\""sv;
    }
    out << ">"sv;
    EncodeText(out, text_);
    out << "</text>"sv;
}

void Document::AddPtr(std::unique_ptr<Object>&& obj) {
    objects_.emplace_back(std::move(obj));
}

// Выводит в ostream svg-представление документа
void Document::Render(std::ostream& out) const {
    out << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>"sv << std::endl;
    out << "<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\">"sv << std::endl;

    RenderContext ctx(out, 2, 2);
    for (const auto& obj : objects_) {
        obj->Render(ctx);
    }

    out << "</svg>\n"sv;
}

}  // namespace svg
