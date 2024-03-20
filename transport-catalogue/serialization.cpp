#include "serialization.h"

#include "domain.h"
#include "geo.h"

#include <unordered_map>
#include <string_view>
#include <fstream>
#include <optional>
#include <sstream>



namespace transport_catalogue_serialize {
	namespace details {
		Color ConvertColorToRaw(const svg::Color& color) {
			Color result;

			result.set_color_type(color.index());

			switch (color.index()) {
			case (1):
				result.set_str_color(std::get<std::string>(color));
				break;
			case (2):
				result.set_red(std::get<svg::Rgb>(color).red);
				result.set_green(std::get<svg::Rgb>(color).green);
				result.set_blue(std::get<svg::Rgb>(color).blue);
				break;
			case (3):
				result.set_red(std::get<svg::Rgba>(color).red);
				result.set_green(std::get<svg::Rgba>(color).green);
				result.set_blue(std::get<svg::Rgba>(color).blue);
				result.set_opacity(std::get<svg::Rgba>(color).opacity);
				break;
			default:
				break;
			}

			return result;
		}


		svg::Color ConvertRawColorToNormal(const Color& raw_color) {

			svg::Color result;

			switch (raw_color.color_type()) {
			case (1):
				result = raw_color.str_color();
				break;
			case (2):
				result = svg::Rgb{ static_cast<uint8_t>(raw_color.red()), static_cast<uint8_t>(raw_color.green()), static_cast<uint8_t>(raw_color.blue()) };
				break;
			case (3):
				result = svg::Rgba{ static_cast<uint8_t>(raw_color.red()), static_cast<uint8_t>(raw_color.green()), static_cast<uint8_t>(raw_color.blue()), raw_color.opacity() };
				break;
			default:
				return svg::NoneColor;
			}

			return result;
		}



		RenderSettings ConvertMapRendererToRaw(const transport_catalogue::renderer::MapRenderer& renderer) {
			RenderSettings result;
			transport_catalogue::renderer::RenderSettings settings = renderer.GetRenderSettings();

			result.set_width(settings.width);
			result.set_height(settings.height);

			result.set_padding(settings.padding);

			result.set_line_width(settings.line_width);
			result.set_stop_radius(settings.stop_radius);

			result.set_bus_label_font_size(settings.bus_label_font_size);
			result.set_bus_label_offset_dx(settings.bus_label_offset.x);
			result.set_bus_label_offset_dy(settings.bus_label_offset.y);

			result.set_stop_label_font_size(settings.stop_label_font_size);
			result.set_stop_label_offset_dx(settings.stop_label_offset.x);
			result.set_stop_label_offset_dy(settings.stop_label_offset.y);

			*(result.mutable_underlayer_color()) = ConvertColorToRaw(settings.underlayer_color);

			result.set_underlayer_width(settings.underlayer_width);

			for (const svg::Color& color : settings.color_palette) {
				*(result.add_palette()) = ConvertColorToRaw(color);
			}

			return result;
		}

		transport_catalogue::renderer::RenderSettings ConvertRawRenderSettingsToNormal(const RenderSettings& raw_settings) {
			transport_catalogue::renderer::RenderSettings result;

			result.width = raw_settings.width();
			result.height = raw_settings.height();

			result.padding = raw_settings.padding();

			result.line_width = raw_settings.line_width();
			result.stop_radius = raw_settings.stop_radius();

			result.bus_label_font_size = raw_settings.bus_label_font_size();
			result.bus_label_offset = svg::Point{
				raw_settings.bus_label_offset_dx(),
				raw_settings.bus_label_offset_dy()
				};

			result.stop_label_font_size = raw_settings.stop_label_font_size();
			result.stop_label_offset = svg::Point{
				raw_settings.stop_label_offset_dx(),
				raw_settings.stop_label_offset_dy()
				};

			result.underlayer_color = transport_catalogue_serialize::details::ConvertRawColorToNormal(raw_settings.underlayer_color());
			result.underlayer_width = raw_settings.underlayer_width();

			for (const Color& raw_color : raw_settings.palette()) {
				result.color_palette.push_back(transport_catalogue_serialize::details::ConvertRawColorToNormal(raw_color));
			}

			return result;
		}


		Graph ConvertGraphToRaw(const graph::DirectedWeightedGraph<transport_catalogue::TransportRouter::RouteWeight>& graph) {
			Graph result;

			for (const std::vector<graph::EdgeId>& ids : graph.GetIncidentLists()) {
				IncidenceList incidence_list;

				for (graph::EdgeId id : ids) {
					incidence_list.add_incidence(id);
				}

				*(result.add_incidence_lists()) = std::move(incidence_list);
			}

			for (const graph::Edge<transport_catalogue::TransportRouter::RouteWeight>& edge : graph.GetEdges()) {
				Edge raw_edge;

				raw_edge.set_from(edge.from);
				raw_edge.set_to(edge.to);

				Weight raw_weight;

				raw_weight.set_type(static_cast<uint32_t>(edge.weight.type));
				raw_weight.set_weight(edge.weight.weight);
				raw_weight.set_name(std::string(edge.weight.name));
				raw_weight.set_span_count(edge.weight.span_count);


				*(raw_edge.mutable_weight()) = std::move(raw_weight);

				*(result.add_edges()) = std::move(raw_edge);
			}

			return result;
		}


		graph::DirectedWeightedGraph<transport_catalogue::TransportRouter::RouteWeight> ConvertRawGraphToNormal(const Graph& raw_graph, const transport_catalogue::Catalogue& catalogue) {
			std::vector<graph::Edge<transport_catalogue::TransportRouter::RouteWeight>> edges;
			edges.reserve(raw_graph.edges().size());


			for (const Edge& raw_edge : raw_graph.edges()) {

				std::string_view stop_name;
				if (transport_catalogue::Stop* stop = catalogue.FindStop(raw_edge.weight().name())) {
					stop_name = stop->name;
				}

				graph::Edge<transport_catalogue::TransportRouter::RouteWeight> edge{
					raw_edge.from(),
					raw_edge.to(),
					transport_catalogue::TransportRouter::RouteWeight {
						static_cast<transport_catalogue::TransportRouter::PassengerActivityType>(raw_edge.weight().type()),
						raw_edge.weight().weight(),
						stop_name,
						raw_edge.weight().span_count()
					}
				};

				edges.push_back(std::move(edge));
			}



			std::vector<std::vector<graph::EdgeId>> incidence_lists;
			incidence_lists.reserve(raw_graph.incidence_lists().size());

			for (const IncidenceList& raw_incidence_list : raw_graph.incidence_lists()) {
				std::vector<graph::EdgeId> incidence_list(raw_incidence_list.incidence().begin(), raw_incidence_list.incidence().end());
				incidence_lists.push_back(std::move(incidence_list));
			}


			 return graph::DirectedWeightedGraph<transport_catalogue::TransportRouter::RouteWeight>{std::move(edges), std::move(incidence_lists)};
		}


		Router ConvertTransportRouterToRaw(const transport_catalogue::TransportRouter& router) {
			Router result;

			*(result.mutable_graph()) = ConvertGraphToRaw(router.GetGraph());

			using CatalogueRouter = graph::Router<transport_catalogue::TransportRouter::RouteWeight>;

			for (const auto& route_iternal_data_list : router.GetRouter().GetRoutesInternalData()) {
				RouteInternalDataList route_internal_data_list;

				for (const std::optional<CatalogueRouter::RouteInternalData>& route_internal_data : route_iternal_data_list) {
					RouteInternalDataOptional route_internal_data_opt;

					if (route_internal_data) {
						RouteInternalData raw_route_internal_data;

						Weight raw_weight;
						raw_weight.set_type(static_cast<uint32_t>(route_internal_data->weight.type));
						raw_weight.set_weight(route_internal_data->weight.weight);
						raw_weight.set_name(std::string(route_internal_data->weight.name));
						raw_weight.set_span_count(route_internal_data->weight.span_count);

						*(raw_route_internal_data.mutable_weight()) = std::move(raw_weight);

						raw_route_internal_data.set_has_prev_edge(route_internal_data->prev_edge.has_value());
						if (route_internal_data->prev_edge)
							raw_route_internal_data.set_prev_edge(*(route_internal_data->prev_edge));

						*(route_internal_data_opt.mutable_route_iternal_data()) = std::move(raw_route_internal_data);
					}

					*(route_internal_data_list.add_route_iternal_data_opt()) = std::move(route_internal_data_opt);
				}

				*(result.add_routes_internal_data()) = std::move(route_internal_data_list);
			}

			for (auto [stop_name, size_t] : router.GetStopNamesToIds()) {
				StopNameToId stop_name_to_id;

				stop_name_to_id.set_stop_name(std::string(stop_name));
				stop_name_to_id.set_id(size_t);


				*(result.add_stop_name_to_id()) = std::move(stop_name_to_id);
			}

			return result;
		}


		transport_catalogue::TransportRouter ConvertRawTransportRouterToNormal(const Router& raw_router, const transport_catalogue::Catalogue& catalogue, graph::DirectedWeightedGraph<transport_catalogue::TransportRouter::RouteWeight>& graph) {
			graph::Router<transport_catalogue::TransportRouter::RouteWeight>::RoutesInternalData routes_internal_data;



			for (const RouteInternalDataList& route_internal_data_list : raw_router.routes_internal_data()) {
				routes_internal_data.push_back({});
				for (const RouteInternalDataOptional& route_internal_data_opt : route_internal_data_list.route_iternal_data_opt()) {
					if (route_internal_data_opt.has_route_iternal_data()) {
						const auto& route_iternal_data = route_internal_data_opt.route_iternal_data();

						std::string_view stop_name;

						if (transport_catalogue::Stop* stop = catalogue.FindStop(route_iternal_data.weight().name())) {
							stop_name = stop->name;
						}

						routes_internal_data.back().push_back(
							graph::Router<transport_catalogue::TransportRouter::RouteWeight>::RouteInternalData{
								transport_catalogue::TransportRouter::RouteWeight {
									transport_catalogue::TransportRouter::PassengerActivityType(route_iternal_data.weight().type()),
									route_iternal_data.weight().weight(),
									stop_name,
									route_iternal_data.weight().span_count()
								},

								std::nullopt
							}
						);



						if (route_iternal_data.has_prev_edge()) {
							routes_internal_data.back().back()->prev_edge = route_iternal_data.prev_edge();
						}

					}
					else
						routes_internal_data.back().push_back(std::nullopt);
				}
			}


			std::unordered_map<std::string_view, size_t> stop_name_to_id;

			for (const StopNameToId& raw_stop_name_to_id : raw_router.stop_name_to_id()) {
				stop_name_to_id[std::string_view(catalogue.FindStop(raw_stop_name_to_id.stop_name())->name)] = raw_stop_name_to_id.id();
			}


			return transport_catalogue::TransportRouter{ catalogue, graph, std::move(routes_internal_data), std::move(stop_name_to_id) };
		}





		TransportCatalogue ConvertCatalogueToRaw(const transport_catalogue::Catalogue& catalogue, const transport_catalogue::renderer::MapRenderer& renderer, const transport_catalogue::TransportRouter& router) {
			TransportCatalogue result;

			result.set_bus_wait_time(catalogue.GetWaitTime());
			result.set_bus_wait_time(catalogue.GetBusVelocity());


			std::unordered_map<const transport_catalogue::Stop*, uint32_t> stops_to_stop_ids;
			uint32_t curent_id = 0;
			for (const transport_catalogue::Stop& stop : catalogue.GetStops()) {
				Stop raw_stop;
				raw_stop.set_name(stop.name);

				raw_stop.set_coordinate_x(stop.coordinates.lat);
				raw_stop.set_coordinate_y(stop.coordinates.lng);

				raw_stop.set_id(curent_id);

				stops_to_stop_ids[&stop] = curent_id;
				++curent_id;

				*(result.add_stop()) = std::move(raw_stop);
			}

			for (const auto& [stops, distance] : catalogue.GetDistances()) {
				DistanceBetweenStops raw_distance;
				raw_distance.set_distance(distance);

				StopsPair raw_stops_pair;
				raw_stops_pair.set_stop_id_1(stops_to_stop_ids.at(stops.first));
				raw_stops_pair.set_stop_id_2(stops_to_stop_ids.at(stops.second));

				*(raw_distance.mutable_stops()) = std::move(raw_stops_pair);
				*(result.add_distances_between_stops()) = std::move(raw_distance);
			}

			for (const transport_catalogue::Bus& bus : catalogue.GetBuses()) {
				Bus raw_bus;
				raw_bus.set_name(bus.name);
				raw_bus.set_is_ring_route(bus.is_ring_route);

				for (const auto& stop : bus.GetStops()) {
					raw_bus.add_stop_id(stops_to_stop_ids.at(stop));
				}

				*(result.add_bus()) = std::move(raw_bus);
			}


			*(result.mutable_render_settings()) = ConvertMapRendererToRaw(renderer);
			*(result.mutable_router()) = ConvertTransportRouterToRaw(router);

			return result;
		}

		transport_catalogue::Catalogue ConvertRawCatalogueToNormal(const TransportCatalogue& catalogue) {
			transport_catalogue::Catalogue result;

			result.SetRoutingSettings(catalogue.bus_wait_time(), catalogue.bus_velocity());


			std::unordered_map<uint32_t, transport_catalogue::Stop*> stop_ids_to_stops;

			for (size_t index = 0; index < catalogue.stop_size(); ++index) {
				const Stop& raw_stop = catalogue.stop(index);

				result.AddStop(
					raw_stop.name(),
					geo::Coordinates{
						raw_stop.coordinate_x(),
						raw_stop.coordinate_y()
					}
				);

				transport_catalogue::Stop* stop = result.FindStop(raw_stop.name());

				stop_ids_to_stops[raw_stop.id()] = stop;
			}


			for (size_t index = 0; index < catalogue.distances_between_stops_size(); ++index) {
				const DistanceBetweenStops& raw_distance = catalogue.distances_between_stops(index);

				transport_catalogue::Stop* stop1 = stop_ids_to_stops[raw_distance.stops().stop_id_1()];
				transport_catalogue::Stop* stop2 = stop_ids_to_stops[raw_distance.stops().stop_id_2()];

				result.AddDistance(
					stop1,
					{ { stop2->name, raw_distance.distance() } }
				);
			}
			for (size_t index = 0; index < catalogue.bus_size(); ++index) {
				const Bus& raw_bus = catalogue.bus(index);

				std::vector<std::string_view> stops_names;
				for (uint32_t stop_id : raw_bus.stop_id()) {
					stops_names.push_back(stop_ids_to_stops[stop_id]->name);
				}

				result.AddBus(
					raw_bus.name(),
					stops_names,
					raw_bus.is_ring_route()
				);

			}



			return result;
		}





		void SerializeRawCatalogue(std::ostream& output, const TransportCatalogue& catalogue) {
			catalogue.SerializeToOstream(&output);
		}

	}




	void Serialize(std::ostream& output, const transport_catalogue::Catalogue& catalogue, const transport_catalogue::renderer::MapRenderer& renderer, const transport_catalogue::TransportRouter& router) {
		details::ConvertCatalogueToRaw(catalogue, renderer, router).SerializeToOstream(&output);
	}

	std::optional<TransportCatalogue> Deserialize(std::istream& input) {
		TransportCatalogue result;
		if (!result.ParseFromIstream(&input)) {
			return std::nullopt;
		}

		return { std::move(result) };
	}


	void Serialize(const std::filesystem::path& output_file, const transport_catalogue::Catalogue& catalogue, const transport_catalogue::renderer::MapRenderer& renderer, const transport_catalogue::TransportRouter& router) {
		std::ofstream output(output_file, std::ios::binary);
		Serialize(output, catalogue, renderer, router);
	}

	std::optional<TransportCatalogue> Deserialize(const std::filesystem::path& input_file) {
		std::ifstream input(input_file, std::ios::binary);
		return Deserialize(input);
	}

}