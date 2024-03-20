#pragma once

#include "geo.h"
#include "domain.h"

#include <string>
#include <string_view>
#include <vector>
#include <set>
#include <deque>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <optional>

namespace transport_catalogue {

	using Minutes = double;
	using BusVelocity = double;

	class Catalogue final {
		struct StopsPairHasher {
			StopsPairHasher();
			const size_t N = 24;
			size_t operator()(std::pair<Stop*, Stop*> stops) const;
		};
	public:

		Catalogue() = default;
		Catalogue(Minutes bus_wait_time, BusVelocity bus_velocity);
		void SetRoutingSettings(Minutes bus_wait_time, BusVelocity bus_velocity);

		//добавление остановки в базу
		void AddStop(const Stop& stop);
		void AddStop(std::string_view stop_name, geo::Coordinates coordinates);

		//добавление маршрута в базу
		void AddBus(std::string_view bus_name, const std::vector<std::string_view>& stops, bool is_ring_route);

		//добавление дистанции в базу
		void AddDistance(Stop* stop, const std::unordered_map<std::string_view, double>& distanses);
		double GetDistance(std::pair<Stop*, Stop*> stops);
		double GetDistance(Stop* stop1, Stop* stop2);

		//поиск остановки по имени
		Stop* FindStop(std::string_view stop_name) const;

		//поиск маршрута по имени,
		Bus* FindBus(std::string_view bus_name) const;

		//получение информации о маршруте
		std::optional<BusInfo> GetBusInfo(std::string_view bus_name) const;

		//получение информации об остановке
		std::optional<StopsBuses> GetBusesByStop(std::string_view stop_name) const;


		size_t GetStopsCount() const;
		size_t GetBusesCount() const;
		const std::deque<Bus>& GetBuses() const;
		const std::deque<Stop>& GetStops() const;
		const std::unordered_map<std::pair<Stop*, Stop*>, double, StopsPairHasher>& GetDistances() const;

		Minutes GetWaitTime() const;
		BusVelocity GetBusVelocity() const;
		Minutes GetBusRideTime(std::pair<Stop*, Stop*> stops) const;
		Minutes GetBusRideTime(Stop* stop1, Stop* stop2) const;


	private:

		Minutes bus_wait_time_ = 0.;
		BusVelocity bus_velocity_ = 1.;

		std::deque<Stop> stops_;
		std::deque<Bus> buses_;
		std::unordered_map<std::string_view, Stop*> stopname_to_stop_;
		std::unordered_map<std::string_view, Bus*> busname_to_bus_;
		std::unordered_map<std::pair<Stop*, Stop*>, double, StopsPairHasher> distance_between_stops_;
		std::unordered_map<std::string_view, std::set<std::string_view>> stop_to_buses_;



		static double KilometresToMetres(double length_in_metres);
		static Minutes HoursToMinutes(double time_in_hours);
	};
}