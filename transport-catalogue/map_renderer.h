#pragma once

#include "geo.h"
#include "domain.h"
#include "svg.h"
#include "transport_catalogue.h"


#include <iostream>
#include <vector>
#include <string_view>
#include <utility>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <algorithm>
#include <cstdlib>
#include <optional>


namespace transport_catalogue::renderer {

    inline const double EPSILON = 1e-6;


    struct RenderSettings {
        double width = 0.;
        double height = 0.;

        double padding = 0.;

        double line_width = 0.;
        double stop_radius = 0.;

        uint32_t bus_label_font_size = 0;
        svg::Point bus_label_offset;

        uint32_t stop_label_font_size = 0;
        svg::Point stop_label_offset;

        svg::Color underlayer_color;
        double underlayer_width = 0.;

        std::vector<svg::Color> color_palette;
    };


    class MapRenderer final {
    public:

        explicit MapRenderer() = default;
        explicit MapRenderer(const Catalogue& catalogue);
        explicit MapRenderer(const Catalogue& catalogue, const RenderSettings& settings);


        void Render(std::ostream& out) const;

        MapRenderer& AddBus(const Bus& bus);


        RenderSettings GetRenderSettings() const;
        MapRenderer& SetRenderSettings(const RenderSettings& settings);


        MapRenderer& SetWidth(double width);
        MapRenderer& SetHeight(double height);

        MapRenderer& SetPadding(double padding);

        MapRenderer& SetLineWidth(double width);
        MapRenderer& SetStopRadius(double radius);

        MapRenderer& SetBusLabelFontSize(uint32_t size);
        MapRenderer& SetBusLabelOffset(double dx, double dy);

        MapRenderer& SetStopLabelFontSize(uint32_t size);
        MapRenderer& SetStopLabelOffset(double dx, double dy);

        MapRenderer& SetUnderlayerColor(svg::Color color);
        MapRenderer& SetUnderlayerWidth(double width);

        MapRenderer& SetColorPalette(std::vector<svg::Color> palette);

    private:
        class SphereProjector;

        struct CoordinatesHasher {
            CoordinatesHasher();
            const double N = 24.;
            size_t operator()(geo::Coordinates coordinates) const;
        };



        std::map<std::string_view, std::vector<geo::Coordinates>> routes_;
        std::unordered_map<std::string_view, bool> is_ring_route_;

        std::map<std::string_view, geo::Coordinates> stops_;
        std::unordered_set<geo::Coordinates, CoordinatesHasher> coordinates_;

        double width_ = 0.;
        double height_ = 0.;

        double padding_ = 0.;

        double line_width_ = 0.;
        double stop_radius_ = 0.;

        uint32_t bus_label_font_size_ = 0;
        svg::Point bus_label_offset_;

        uint32_t stop_label_font_size_ = 0;
        svg::Point stop_label_offset_;

        svg::Color underlayer_color_;
        double underlayer_width_ = 0.;

        std::vector<svg::Color> color_palette_;



        template <typename CoordinatesIt>
        void static AddStopsToRoute(CoordinatesIt begin, CoordinatesIt end, svg::Polyline& route, const SphereProjector& proj);


        void RenderRouteLines(svg::Document& doc, const SphereProjector& proj) const;
        void RenderRouteNames(svg::Document& doc, const SphereProjector& proj) const;
        void RenderStopCircles(svg::Document& doc, const SphereProjector& proj) const;
        void RenderStopNames(svg::Document& doc, const SphereProjector& proj) const;




        bool static IsZero(double value);

        class SphereProjector {
        public:
            // points_begin и points_end задают начало и конец интервала элементов geo::Coordinates
            template <typename PointInputIt>
            SphereProjector(PointInputIt points_begin, PointInputIt points_end, double max_width, double max_height, double padding);

            // Проецирует широту и долготу в координаты внутри SVG-изображения
            svg::Point operator()(geo::Coordinates coords) const;

        private:
            double padding_;
            double min_lon_ = 0;
            double max_lat_ = 0;
            double zoom_coeff_ = 0;
        };

    };









    // ---------- Implementations ------------------

    template <typename PointInputIt>
    MapRenderer::SphereProjector::SphereProjector(PointInputIt points_begin, PointInputIt points_end, double max_width, double max_height, double padding) : padding_(padding) {

        // Если точки поверхности сферы не заданы, вычислять нечего
        if (points_begin == points_end) {
            return;
        }

        // Находим точки с минимальной и максимальной долготой
        const auto [left_it, right_it] = std::minmax_element(
            points_begin, points_end,
            [](auto lhs, auto rhs) { return lhs.lng < rhs.lng; });

        min_lon_ = left_it->lng;
        const double max_lon = right_it->lng;


        // Находим точки с минимальной и максимальной широтой
        const auto [bottom_it, top_it] = std::minmax_element(
            points_begin, points_end,
            [](auto lhs, auto rhs) { return lhs.lat < rhs.lat; });

        const double min_lat = bottom_it->lat;
        max_lat_ = top_it->lat;



        // Вычисляем коэффициент масштабирования вдоль координаты x
        std::optional<double> width_zoom;
        if (!IsZero(max_lon - min_lon_)) {
            width_zoom = (max_width - 2 * padding) / (max_lon - min_lon_);
        }

        // Вычисляем коэффициент масштабирования вдоль координаты y
        std::optional<double> height_zoom;
        if (!IsZero(max_lat_ - min_lat)) {
            height_zoom = (max_height - 2 * padding) / (max_lat_ - min_lat);
        }


        if (width_zoom && height_zoom) {
            // Коэффициенты масштабирования по ширине и высоте ненулевые,
            // берём минимальный из них
            zoom_coeff_ = std::min(*width_zoom, *height_zoom);
        }
        else if (width_zoom) {
            // Коэффициент масштабирования по ширине ненулевой, используем его
            zoom_coeff_ = *width_zoom;
        }
        else if (height_zoom) {
            // Коэффициент масштабирования по высоте ненулевой, используем его
            zoom_coeff_ = *height_zoom;
        }
    }



    template <typename CoordinatesIt>
    void MapRenderer::AddStopsToRoute(CoordinatesIt begin, CoordinatesIt end, svg::Polyline& route, const SphereProjector& proj) {
        for (; begin != end; ++begin) {
            route.AddPoint(proj(*begin));
        }
    }
}