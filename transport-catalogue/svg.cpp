#include "svg.h"

namespace svg {

    using namespace std::literals;


    // ---------- Colors ------------------

    Rgb::Rgb(uint8_t r, uint8_t g, uint8_t b) : red(r), green(g), blue(b) {}
    Rgba::Rgba(uint8_t r, uint8_t g, uint8_t b, double alpha) : Rgb(r, g, b), opacity(alpha) {}


    void AttributeColorPrinter::operator()(std::monostate) const {
        using namespace std::literals;
        out << "none"sv;
    }
    void AttributeColorPrinter::operator()(const std::string& color) const {
        out << color;
    }
    void AttributeColorPrinter::operator()(Rgb color) const {
        using namespace std::literals;
        out << "rgb("sv << static_cast<int>(color.red) << ',' << static_cast<int>(color.green) << ',' << static_cast<int>(color.blue) << ")"sv;
    }
    void AttributeColorPrinter::operator()(Rgba color) const {
        using namespace std::literals;

        out << "rgba("sv <<
            static_cast<int>(color.red) << ',' <<
            static_cast<int>(color.green) << ',' <<
            static_cast<int>(color.blue) << ','
            << color.opacity << ")"sv;
    }


    std::ostream& operator<<(std::ostream& out, Color color) {
        std::visit(AttributeColorPrinter{ out }, color);
        return out;
    }



    // ---------- Output ------------------

    std::ostream& operator<<(std::ostream& out, StrokeLineCap stroke_line_cap) {
        switch (stroke_line_cap) {
        case StrokeLineCap::BUTT:
            out << "butt"s;
            break;
        case StrokeLineCap::ROUND:
            out << "round"s;
            break;
        case StrokeLineCap::SQUARE:
            out << "square"s;
            break;
        }

        return out;
    }

    std::ostream& operator<<(std::ostream& out, StrokeLineJoin stroke_line_join) {

        switch (stroke_line_join) {
        case svg::StrokeLineJoin::ARCS:
            out << "arcs"s;
            break;
        case svg::StrokeLineJoin::BEVEL:
            out << "bevel"s;
            break;
        case svg::StrokeLineJoin::MITER:
            out << "miter"s;
            break;
        case svg::StrokeLineJoin::MITER_CLIP:
            out << "miter-clip"s;
            break;
        case svg::StrokeLineJoin::ROUND:
            out << "round"s;
            break;
        }

        return out;
    }





    // ---------- Object ------------------

    void Object::Render(const RenderContext& context) const {
        context.RenderIndent();

        // Делегируем вывод тега своим подклассам
        RenderObject(context);

        context.out << std::endl;
    }




    // ---------- Circle ------------------

    Circle& Circle::SetCenter(Point center) {
        center_ = center;
        return *this;
    }

    Circle& Circle::SetRadius(double radius) {
        radius_ = radius;
        return *this;
    }

    void Circle::RenderObject(const RenderContext& context) const {
        auto& out = context.out;
        out << "<circle cx=\""sv << center_.x << "\" cy=\""sv << center_.y << "\" "sv;
        out << "r=\""sv << radius_ << "\""sv;
        RenderAttrs(out);
        out << "/>"sv;
    }



    // ---------- Polyline ------------------

    Polyline& Polyline::AddPoint(Point point) {
        points_.push_back(point);
        return *this;
    }

    void Polyline::RenderObject(const RenderContext& context) const {
        auto& out = context.out;
        out << "<polyline points=\""sv;
        for (const Point& point : points_) {
            out << point.x << ',' << point.y << (&point == &points_.back() ? ""sv : " "sv);
        }
        out << "\""sv;

        RenderAttrs(out);
        out << "/>"sv;
    }



    // ---------- Text ------------------

    Text& Text::SetPosition(Point pos) {
        pos_ = pos;
        return *this;
    }

    // Задаёт смещение относительно опорной точки (атрибуты dx, dy)
    Text& Text::SetOffset(Point offset) {
        offset_ = offset;
        return *this;
    }

    // Задаёт размеры шрифта (атрибут font-size)
    Text& Text::SetFontSize(uint32_t size) {
        font_size_ = size;
        return *this;
    }

    // Задаёт название шрифта (атрибут font-family)
    Text& Text::SetFontFamily(std::string font_family) {
        font_family_ = std::move(font_family);
        return *this;
    }

    // Задаёт толщину шрифта (атрибут font-weight)
    Text& Text::SetFontWeight(std::string font_weight) {
        font_weight_ = std::move(font_weight);
        return *this;
    }

    // Задаёт текстовое содержимое объекта (отображается внутри тега text)
    Text& Text::SetData(std::string data) {
        ReplaceAllCharacters(data, '&', "&amp;"s);
        ReplaceAllCharacters(data, '"', "&quot;"s);
        ReplaceAllCharacters(data, '\'', "&apos;"s);
        ReplaceAllCharacters(data, '<', "&lt;"s);
        ReplaceAllCharacters(data, '>', "&gt;"s);

        data_ = std::move(data);
        return *this;
    }

    void Text::RenderObject(const RenderContext& context) const {
        auto& out = context.out;
        out << "<text"sv;
        RenderAttrs(out);

        out << " x=\""sv << pos_.x << "\" y=\""sv << pos_.y
            << "\" dx=\""sv << offset_.x << "\" dy=\""sv << offset_.y
            << "\" font-size=\""sv << font_size_;

        if (!font_family_.empty())
            out << "\" font-family=\""sv << font_family_;
        if (!font_weight_.empty())
            out << "\" font-weight=\""sv << font_weight_;


        out << "\">"sv << data_
            << "</text>"sv;
    }


    void Text::ReplaceAllCharacters(std::string& str, char char_to_replace, const std::string& replacement) {
        size_t place = str.find(char_to_replace);
        while (place != str.npos) {
            str.replace(place, 1, replacement);
            place = str.find(char_to_replace, (place + 1));
        }
    }






    // ---------- Document ------------------

    // Добавляет в svg-документ объект-наследник svg::Object
    void Document::AddPtr(std::unique_ptr<Object>&& obj) {
        objects_.push_back(std::move(obj));
    }

    // Выводит в ostream svg-представление документа
    void Document::Render(std::ostream& out) const {
        RenderContext context(out, 2);

        out << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>"sv << std::endl;
        out << "<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\">"sv << std::endl;

        for (const auto& obj : objects_) {
            obj->Render(context.Indented());
        }
        out << "</svg>"sv;
    }

}  // namespace svg