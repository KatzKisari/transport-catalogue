#include "map_renderer.h"



namespace transport_catalogue::renderer {

	bool MapRenderer::IsZero(double value) {
		return std::abs(value) < EPSILON;
	}

	svg::Point MapRenderer::SphereProjector::operator()(geo::Coordinates coords) const {
		return {
			(coords.lng - min_lon_) * zoom_coeff_ + padding_,
			(max_lat_ - coords.lat) * zoom_coeff_ + padding_
		};
	}


	MapRenderer::CoordinatesHasher::CoordinatesHasher() : N(24.) { }

	size_t MapRenderer::CoordinatesHasher::operator()(geo::Coordinates coordinates) const {
		return std::abs(static_cast<int>(10000 * (coordinates.lat + coordinates.lng * N)));
	}



	MapRenderer::MapRenderer(const Catalogue& catalogue) {
		for (const Bus& bus : catalogue.GetBuses()) {
			AddBus(bus);
		}
	}

	MapRenderer::MapRenderer(const Catalogue& catalogue, const RenderSettings& settings) : MapRenderer(catalogue) {
		SetRenderSettings(settings);
	}

	void MapRenderer::Render(std::ostream& out) const {
		using namespace std::literals;

		SphereProjector proj(coordinates_.begin(), coordinates_.end(), width_, height_, padding_);

		svg::Document doc;

		// 1. ломаные линии маршрутов
		RenderRouteLines(doc, proj);
		// 2. названия маршрутов
		RenderRouteNames(doc, proj);
		// 3. круги, обозначающие остановки
		RenderStopCircles(doc, proj);
		// 4. названия остановок
		RenderStopNames(doc, proj);


		doc.Render(out);
	}



	void MapRenderer::RenderRouteLines(svg::Document& doc, const SphereProjector& proj) const {
		using namespace std::literals;

		auto color = color_palette_.begin();
		for (const auto& [bus, coordinates] : routes_) {
			svg::Polyline route;

			route
				.SetStrokeWidth(line_width_)
				.SetFillColor("none"s)
				.SetStrokeLineCap(svg::StrokeLineCap::ROUND)
				.SetStrokeLineJoin(svg::StrokeLineJoin::ROUND)
				.SetStrokeColor(*color);

			AddStopsToRoute(coordinates.begin(), coordinates.end(), route, proj);
			if (!is_ring_route_.at(bus)) AddStopsToRoute(std::next(coordinates.rbegin()), coordinates.rend(), route, proj);

			doc.Add(route);

			color = (std::next(color) == color_palette_.end() ? color_palette_.begin() : std::next(color));
		}
	}



	void MapRenderer::RenderRouteNames(svg::Document& doc, const SphereProjector& proj) const {
		using namespace std::literals;

		auto color = color_palette_.begin();
		for (const auto& [bus, coordinates] : routes_) {
			svg::Text route_name_ul;

			route_name_ul
				.SetData(std::string(bus))
				.SetPosition(proj(coordinates.front()))
				.SetOffset(bus_label_offset_)
				.SetFontSize(bus_label_font_size_)
				.SetFontFamily("Verdana"s)
				.SetFontWeight("bold"s);


			svg::Text route_name = route_name_ul;
			route_name.SetFillColor(*color);


			route_name_ul
				.SetFillColor(underlayer_color_)
				.SetStrokeColor(underlayer_color_)
				.SetStrokeWidth(underlayer_width_)
				.SetStrokeLineCap(svg::StrokeLineCap::ROUND)
				.SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);



			doc.Add(route_name_ul);
			doc.Add(route_name);


			if (coordinates.front() != coordinates.back()) {
				doc.Add(route_name_ul.SetPosition(proj(coordinates.back())));
				doc.Add(route_name.SetPosition(proj(coordinates.back())));
			}


			color = (std::next(color) == color_palette_.end() ? color_palette_.begin() : std::next(color));
		}
	}



	void MapRenderer::RenderStopCircles(svg::Document& doc, const SphereProjector& proj) const {
		using namespace std::literals;

		for (const auto& [stop, coordinates] : stops_) {
			svg::Circle stop_circle;

			stop_circle
				.SetCenter(proj(coordinates))
				.SetRadius(stop_radius_)
				.SetFillColor("white"s);

			doc.Add(stop_circle);
		}
	}



	void MapRenderer::RenderStopNames(svg::Document& doc, const SphereProjector& proj) const {
		using namespace std::literals;


		for (const auto& [stop, coordinates] : stops_) {
			svg::Text stop_name_ul;

			stop_name_ul
				.SetData(std::string(stop))
				.SetPosition(proj(coordinates))
				.SetOffset(stop_label_offset_)
				.SetFontSize(stop_label_font_size_)
				.SetFontFamily("Verdana"s);


			svg::Text stop_name = stop_name_ul;
			stop_name.SetFillColor("black"s);


			stop_name_ul
				.SetFillColor(underlayer_color_)
				.SetStrokeColor(underlayer_color_)
				.SetStrokeWidth(underlayer_width_)
				.SetStrokeLineCap(svg::StrokeLineCap::ROUND)
				.SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);



			doc.Add(stop_name_ul);
			doc.Add(stop_name);
		}
	}








	MapRenderer& MapRenderer::AddBus(const Bus& bus) {
		if (bus.stops.empty()) {
			return *this;
		}

		is_ring_route_[bus.name] = bus.is_ring_route;


		std::vector<geo::Coordinates>& stops_on_route = routes_[bus.name];

		const Stop& first_stop = *(bus.keys_for_distance.front().first);

		stops_[first_stop.name] = first_stop.coordinates;
		stops_on_route.push_back(first_stop.coordinates);
		coordinates_.insert(first_stop.coordinates);

		for (const auto& [stop1, stop2] : bus.keys_for_distance) {
			stops_[stop2->name] = stop2->coordinates;
			stops_on_route.push_back(stop2->coordinates);
			coordinates_.insert(stop2->coordinates);
		}


		return *this;
	}


	RenderSettings MapRenderer::GetRenderSettings() const {
		RenderSettings result
		{ width_
		, height_
		, padding_
		, line_width_
		, stop_radius_
		, bus_label_font_size_
		, bus_label_offset_
		, stop_label_font_size_
		, stop_label_offset_
		, underlayer_color_
		, underlayer_width_
		, color_palette_ };

		return result;
	}
	MapRenderer& MapRenderer::SetRenderSettings(const RenderSettings& settings) {
		SetWidth(settings.width);
		SetHeight(settings.height);
		SetPadding(settings.padding);
		SetLineWidth(settings.line_width);
		SetStopRadius(settings.stop_radius);
		SetBusLabelFontSize(settings.bus_label_font_size);
		SetBusLabelOffset(settings.bus_label_offset.x, settings.bus_label_offset.y);
		SetStopLabelFontSize(settings.stop_label_font_size);
		SetStopLabelOffset(settings.stop_label_offset.x, settings.stop_label_offset.y);
		SetUnderlayerColor(settings.underlayer_color);
		SetUnderlayerWidth(settings.underlayer_width);
		SetColorPalette(settings.color_palette);

		return *this;
	}




	MapRenderer& MapRenderer::SetWidth(double width) {
		width_ = width;
		return *this;
	}
	MapRenderer& MapRenderer::SetHeight(double height) {
		height_ = height;
		return *this;
	}

	MapRenderer& MapRenderer::SetPadding(double padding) {
		padding_ = padding;
		return *this;
	}

	MapRenderer& MapRenderer::SetLineWidth(double width) {
		line_width_ = width;
		return *this;
	}
	MapRenderer& MapRenderer::SetStopRadius(double radius) {
		stop_radius_ = radius;
		return *this;
	}

	MapRenderer& MapRenderer::SetBusLabelFontSize(uint32_t size) {
		bus_label_font_size_ = size;
		return *this;
	}
	MapRenderer& MapRenderer::SetBusLabelOffset(double dx, double dy) {
		bus_label_offset_ = svg::Point{ dx, dy };
		return *this;
	}

	MapRenderer& MapRenderer::SetStopLabelFontSize(uint32_t size) {
		stop_label_font_size_ = size;
		return *this;
	}
	MapRenderer& MapRenderer::SetStopLabelOffset(double dx, double dy) {
		stop_label_offset_ = svg::Point{ dx, dy };
		return *this;
	}

	MapRenderer& MapRenderer::SetUnderlayerColor(svg::Color color) {
		underlayer_color_ = color;
		return *this;
	}
	MapRenderer& MapRenderer::SetUnderlayerWidth(double width) {
		underlayer_width_ = width;
		return *this;
	}

	MapRenderer& MapRenderer::SetColorPalette(std::vector<svg::Color> palette) {
		color_palette_ = palette;
		return *this;
	}

}