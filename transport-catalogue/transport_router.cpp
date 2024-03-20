#include "transport_router.h"


namespace transport_catalogue {


	TransportRouter::TransportRouter(const Catalogue& catalogue) : catalogue_(catalogue), stop_name_to_id_(), router_(InitGraph()) { }

	TransportRouter::TransportRouter(const Catalogue& catalogue
		, graph::DirectedWeightedGraph<RouteWeight>& graph
		, graph::Router<RouteWeight>::RoutesInternalData&& routes_internal_data
		, std::unordered_map<std::string_view, size_t>&& stop_name_to_id)
		: catalogue_(catalogue),
		graph_(&graph),
		router_(*graph_, std::move(routes_internal_data)),
		stop_name_to_id_(stop_name_to_id)
	{ }

	json::Node TransportRouter::BuildRoute(const json::Dict& query) const {
		return BuildRoute(query.at("from").AsString(), query.at("to").AsString());
	}

	json::Node TransportRouter::BuildRoute(std::string_view stop_name_from, std::string_view stop_name_to) const {
		using namespace std::literals;

		json::Dict result{};

		std::optional<graph::Router<RouteWeight>::RouteInfo> route
			= router_.BuildRoute(
				stop_name_to_id_.at(stop_name_from) * 2
				, stop_name_to_id_.at(stop_name_to) * 2);

		if (!route) {
			result["error_message"s] = "not found"s;
			return json::Node{ result };
		}

		result["total_time"s] = route->weight.weight;

		json::Array items{};


		for (graph::EdgeId edge_id : route->edges) {
			const graph::Edge<RouteWeight>& edge = graph_->GetEdge(edge_id);
			json::Dict item{ {"time"s, edge.weight.weight} };


			switch (edge.weight.type) {
			case (PassengerActivityType::WAIT):
				item["type"s] = "Wait"s;
				item["stop_name"s] = std::string(edge.weight.name);

				break;

			case (PassengerActivityType::BUS):
				item["type"s] = "Bus"s;
				item["bus"s] = std::string(edge.weight.name);
				item["span_count"s] = static_cast<int>(edge.weight.span_count);

				break;
			case (PassengerActivityType::MIXED):
				throw(std::invalid_argument("..."s));
				break;
			}


			items.push_back(std::move(item));
		}

		result["items"s] = std::move(items);

		return json::Node{ std::move(result) };
	}



	const std::unordered_map<std::string_view, size_t>& TransportRouter::GetStopNamesToIds() const {
		return stop_name_to_id_;
	}
	const graph::DirectedWeightedGraph<transport_catalogue::TransportRouter::RouteWeight>& TransportRouter::GetGraph() const {
		return *graph_;
	}
	const graph::Router<transport_catalogue::TransportRouter::RouteWeight>& TransportRouter::GetRouter() const {
		return router_;
	}




	const Stop& TransportRouter::GetStopById(size_t id) const {
		return catalogue_.GetStops()[id];
	}


	graph::DirectedWeightedGraph<TransportRouter::RouteWeight>& TransportRouter::InitGraph() {
		graph::DirectedWeightedGraph<RouteWeight> graph(catalogue_.GetStopsCount() * 2);

		//enter to exit edges 
		for (size_t i = 0; i < catalogue_.GetStopsCount(); ++i) {
			std::string_view stop_name = GetStopById(i).name;
			graph.AddEdge({ i * 2, i * 2 + 1, { PassengerActivityType::WAIT, catalogue_.GetWaitTime(), stop_name, 0} });
			stop_name_to_id_[stop_name] = i;
		}


		for (const Bus& bus : catalogue_.GetBuses()) {
			auto stops = bus.GetStops();
			BuildRoadsFromStopOnGraph(stops.begin(), (bus.is_ring_route ? std::prev(stops.end()) : stops.end()), graph, bus);

			for (auto stop_it = std::next(stops.begin()); stop_it != stops.end(); ++stop_it) {
				BuildRoadsFromStopOnGraph(stop_it, stops.end(), graph, bus);
			}
		}

		graph_ = new graph::DirectedWeightedGraph<TransportRouter::RouteWeight>(std::move(graph));

		return *graph_;
	}

}