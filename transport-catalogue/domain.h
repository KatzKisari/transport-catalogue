#pragma once


#include "geo.h"

#include <string>
#include <string_view>
#include <vector>
#include <set>
#include <unordered_set>


namespace transport_catalogue {

	struct Stop {
		std::string name;
		geo::Coordinates coordinates;

		bool operator==(const Stop& rhs) const;
	};

	struct Bus {
		std::string name;
		std::unordered_set<Stop*> stops;
		std::vector<std::pair<Stop*, Stop*>> keys_for_distance;
		double geo_distance = 0.0;
		bool is_ring_route = false;

		std::vector<Stop*> GetStops() const;


		bool operator==(const Bus& rhs) const;

		bool operator<(const Bus& rhs) const;
		bool operator>(const Bus& rhs) const;

		bool operator<=(const Bus& rhs) const;
		bool operator>=(const Bus& rhs) const;
	};



	struct BusInfo {
		std::string_view name;
		size_t stops_count = 0;
		size_t unique_stops_count = 0;
		double route_length = 0.0;
		double curvature = 0.0;
	};

	using StopsBuses = std::set<std::string_view>;
}