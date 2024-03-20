#pragma once

#include "graph.h"
#include "router.h"
#include "transport_catalogue.h"
#include "json.h"

#include <string_view>
#include <unordered_map>
#include <vector>

#include <utility>

namespace transport_catalogue {


	class TransportRouter {
	public:
		enum class PassengerActivityType {
			WAIT,
			BUS,
			MIXED
		};

		struct RouteWeight {
			PassengerActivityType type;
			double weight = 0;
			std::string_view name;
			size_t span_count = 0;

			inline bool operator>(const RouteWeight& other) const;
			inline bool operator<(const RouteWeight& other) const;
			inline bool operator==(const RouteWeight& other) const;
			inline bool operator>=(const RouteWeight& other) const;
			inline bool operator<=(const RouteWeight& other) const;
			inline RouteWeight operator+(const RouteWeight& other) const;
		};


		TransportRouter(const Catalogue& catalogue);
		TransportRouter(
			const Catalogue& catalogue, 
			graph::DirectedWeightedGraph<RouteWeight>& graph, 
			graph::Router<RouteWeight>::RoutesInternalData&& routes_internal_data, 
			std::unordered_map<std::string_view, size_t>&& stop_name_to_id);

		json::Node BuildRoute(const json::Dict& query) const;
		json::Node BuildRoute(std::string_view stop_name_from, std::string_view stop_name_to) const;


		const std::unordered_map<std::string_view, size_t>& GetStopNamesToIds() const;
		const graph::DirectedWeightedGraph<RouteWeight>& GetGraph() const;
		const graph::Router<RouteWeight>& GetRouter() const;


	private:
		const Catalogue& catalogue_;
		std::unordered_map<std::string_view, size_t> stop_name_to_id_;

		graph::DirectedWeightedGraph<RouteWeight>* graph_;
		graph::Router<RouteWeight> router_;

		static constexpr RouteWeight ZERO_WEIGHT{ PassengerActivityType::WAIT, 0, "", 0};


		const Stop& GetStopById(size_t id) const;

		



		template <typename StopsIterator>
		double CalculateBusRideTime(StopsIterator route_begin, StopsIterator route_end) const;

		template <typename StopsIterator>
		void BuildRoadsFromStopOnGraph(StopsIterator begin, StopsIterator end, graph::DirectedWeightedGraph<RouteWeight>& graph, const Bus& bus) const;
		

		graph::DirectedWeightedGraph<RouteWeight>& InitGraph();
	};















	// ----------------------- implementations -----------------------



	// ********* RouteWeight *********

	inline bool TransportRouter::RouteWeight::operator>(const RouteWeight& other) const {
		return weight > other.weight;
	}

	inline bool TransportRouter::RouteWeight::operator<(const RouteWeight& other) const {
		return weight < other.weight;
	}

	inline bool TransportRouter::RouteWeight::operator==(const RouteWeight& other) const {
		return weight == other.weight;
	}

	inline bool TransportRouter::RouteWeight::operator>=(const RouteWeight& other) const {
		return weight >= other.weight;
	}

	inline bool TransportRouter::RouteWeight::operator<=(const RouteWeight& other) const {
		return weight <= other.weight;
	}

	inline TransportRouter::RouteWeight TransportRouter::RouteWeight::operator+(const RouteWeight& other) const {
		if (*this == ZERO_WEIGHT)
			return other;
		else if (other == ZERO_WEIGHT)
			return *this;


		return { PassengerActivityType::MIXED
			, weight + other.weight
			, std::string_view()
			, span_count + other.span_count };
	}






	// ********* TransportRouter *********


	template <typename StopsIterator>
	double TransportRouter::CalculateBusRideTime(StopsIterator route_begin, StopsIterator route_end) const {
		double result = 0.;

		for (StopsIterator it = std::next(route_begin), prev_stop = route_begin; it != route_end; ++it, ++prev_stop) {
			result += catalogue_.GetBusRideTime(*prev_stop, *it);
		}

		return result;
	}



	template <typename StopsIterator>
	void TransportRouter::BuildRoadsFromStopOnGraph(StopsIterator begin, StopsIterator end, graph::DirectedWeightedGraph<RouteWeight>& graph, const Bus& bus) const {
		StopsIterator curent_stop = begin;
		size_t curent_stop_id = stop_name_to_id_.at((*curent_stop)->name);
		size_t span_count = 0;

		for (StopsIterator stop_it = std::next(curent_stop); stop_it != end; ++stop_it) {
			++span_count;

			Stop* destination_stop = *stop_it;
			size_t destination_stop_id = stop_name_to_id_.at(destination_stop->name);

			graph.AddEdge({ curent_stop_id * 2 + 1, destination_stop_id * 2, {
				PassengerActivityType::BUS
				, CalculateBusRideTime(curent_stop, std::next(stop_it))
				, bus.name
				, span_count } });

			if (!bus.is_ring_route) {
				graph.AddEdge({ destination_stop_id * 2 + 1, curent_stop_id * 2, {
					PassengerActivityType::BUS
					, CalculateBusRideTime(std::make_reverse_iterator(std::next(stop_it)), std::make_reverse_iterator(curent_stop))
					, bus.name
					, span_count } });
			}
		}
	}

}