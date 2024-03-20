#pragma once

#include <cstdint>
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <optional>
#include <variant>

namespace svg {

    struct Rgb {
        Rgb(uint8_t r, uint8_t g, uint8_t b);
        Rgb() = default;

        uint8_t red = 0;
        uint8_t green = 0;
        uint8_t blue = 0;
    };

    struct Rgba : Rgb {
        Rgba(uint8_t r, uint8_t g, uint8_t b, double alpha);
        Rgba() = default;

        double opacity = 1.0;
    };


    using Color = std::variant<std::monostate, std::string, Rgb, Rgba>;

    inline const std::monostate NoneColor;

    struct AttributeColorPrinter {
        std::ostream& out;

        void operator()(std::monostate) const;
        void operator()(const std::string& color) const;
        void operator()(Rgb color) const;
        void operator()(Rgba color) const;
    };


    std::ostream& operator<<(std::ostream& out, Color color);



    struct Point {
        Point() = default;
        Point(double x, double y)
            : x(x)
            , y(y) {
        }
        double x = 0;
        double y = 0;
    };


    enum class StrokeLineCap {
        BUTT,
        ROUND,
        SQUARE,
    };

    std::ostream& operator<<(std::ostream& out, StrokeLineCap stroke_line_cap);


    enum class StrokeLineJoin {
        ARCS,
        BEVEL,
        MITER,
        MITER_CLIP,
        ROUND,
    };

    std::ostream& operator<<(std::ostream& out, StrokeLineJoin stroke_line_join);





    template <typename Owner>
    class PathProps {
    public:
        Owner& SetFillColor(Color color);
        Owner& SetStrokeColor(Color color);

        Owner& SetStrokeWidth(double width);
        Owner& SetStrokeLineCap(StrokeLineCap line_cap);
        Owner& SetStrokeLineJoin(StrokeLineJoin line_join);

    protected:
        void RenderAttrs(std::ostream& out) const;

    private:
        Owner& AsOwner();

        std::optional<Color> fill_color_;
        std::optional<Color> stroke_color_;

        std::optional<double> stroke_width_;
        std::optional<StrokeLineCap> stroke_line_cap_;
        std::optional<StrokeLineJoin> stroke_line_join_;
    };







    /*
     * Вспомогательная структура, хранящая контекст для вывода SVG-документа с отступами.
     * Хранит ссылку на поток вывода, текущее значение и шаг отступа при выводе элемента
     */
    struct RenderContext {
        RenderContext(std::ostream& out)
            : out(out) {
        }

        RenderContext(std::ostream& out, int indent_step, int indent = 0)
            : out(out)
            , indent_step(indent_step)
            , indent(indent) {
        }

        RenderContext Indented() const {
            return { out, indent_step, indent + indent_step };
        }

        void RenderIndent() const {
            for (int i = 0; i < indent; ++i) {
                out.put(' ');
            }
        }

        std::ostream& out;
        int indent_step = 0;
        int indent = 0;
    };









    // Абстрактный базовый класс Object служит для унифицированного хранения конкретных тегов SVG-документа
    class Object {
    public:
        void Render(const RenderContext& context) const;

        virtual ~Object() = default;

    private:
        virtual void RenderObject(const RenderContext& context) const = 0;
    };




    // Класс Circle моделирует элемент <circle> для отображения круга
    class Circle : public Object, public PathProps<Circle> {
    public:
        Circle& SetCenter(Point center);
        Circle& SetRadius(double radius);


    private:
        void RenderObject(const RenderContext& context) const override;

        Point center_;
        double radius_ = 1.0;
    };



    // Класс Polyline моделирует элемент <polyline> для отображения ломаных линий
    class Polyline : public Object, public PathProps<Polyline> {
    public:
        // Добавляет очередную вершину к ломаной линии
        Polyline& AddPoint(Point point);

    private:
        void RenderObject(const RenderContext& context) const override;

        std::vector<Point> points_;
    };




    // Класс Text моделирует элемент <text> для отображения текста
    class Text : public Object, public PathProps<Text> {
    public:
        // Задаёт координаты опорной точки (атрибуты x и y)
        Text& SetPosition(Point pos);

        // Задаёт смещение относительно опорной точки (атрибуты dx, dy)
        Text& SetOffset(Point offset);

        // Задаёт размеры шрифта (атрибут font-size)
        Text& SetFontSize(uint32_t size);

        // Задаёт название шрифта (атрибут font-family)
        Text& SetFontFamily(std::string font_family);

        // Задаёт толщину шрифта (атрибут font-weight)
        Text& SetFontWeight(std::string font_weight);

        // Задаёт текстовое содержимое объекта (отображается внутри тега text)
        Text& SetData(std::string data);

    private:
        void RenderObject(const RenderContext& context) const override;

        static void ReplaceAllCharacters(std::string& str, char char_to_replace, const std::string& replacement);

        Point pos_;
        Point offset_;
        uint32_t font_size_ = 1;
        std::string font_family_;
        std::string font_weight_;
        std::string data_;
    };





    class ObjectContainer {   // * interface
    public:
        template <typename Obj>
        void Add(Obj obj);

        // Добавляет в svg-документ объект-наследник svg::Object
        virtual void AddPtr(std::unique_ptr<Object>&& obj) = 0;
    };



    class Document final : public ObjectContainer {
    public:
        // Добавляет в svg-документ объект-наследник svg::Object
        void AddPtr(std::unique_ptr<Object>&& obj) override;

        // Выводит в ostream svg-представление документа
        void Render(std::ostream& out) const;

    private:
        std::vector<std::unique_ptr<const Object>> objects_;
    };






    class Drawable {   // * interface
    public:
        virtual void Draw(ObjectContainer& container) const = 0;

        virtual ~Drawable() = default;
    };






    // ---------- Implementations ------------------


    template <typename Owner>
    Owner& PathProps<Owner>::AsOwner() {
        return static_cast<Owner&>(*this);
    }




    template <typename Owner>
    Owner& PathProps<Owner>::SetFillColor(Color color) {
        fill_color_ = std::move(color);
        return AsOwner();
    }

    template <typename Owner>
    Owner& PathProps<Owner>::SetStrokeColor(Color color) {
        stroke_color_ = std::move(color);
        return AsOwner();
    }


    template <typename Owner>
    Owner& PathProps<Owner>::SetStrokeWidth(double width) {
        stroke_width_ = std::move(width);
        return AsOwner();
    }

    template <typename Owner>
    Owner& PathProps<Owner>::SetStrokeLineCap(StrokeLineCap line_cap) {
        stroke_line_cap_ = std::move(line_cap);
        return AsOwner();
    }

    template <typename Owner>
    Owner& PathProps<Owner>::SetStrokeLineJoin(StrokeLineJoin line_join) {
        stroke_line_join_ = std::move(line_join);
        return AsOwner();
    }





    template <typename Owner>
    void PathProps<Owner>::RenderAttrs(std::ostream& out) const {
        using namespace std::literals;

        if (fill_color_) {
            out << " fill=\""sv << *fill_color_ << '\"';
        }
        if (stroke_color_) {
            out << " stroke=\""sv << *stroke_color_ << '\"';
        }
        if (stroke_width_) {
            out << " stroke-width=\""sv << *stroke_width_ << '\"';
        }
        if (stroke_line_cap_) {
            out << " stroke-linecap=\""sv << *stroke_line_cap_ << '\"';
        }
        if (stroke_line_join_) {
            out << " stroke-linejoin=\""sv << *stroke_line_join_ << '\"';
        }
    }







    template <typename Obj>
    void ObjectContainer::Add(Obj obj) {
        AddPtr(std::make_unique<Obj>(std::move(obj)));
    }


}  // namespace svg