#include "transport_catalogue.h"

using namespace std;



namespace transport_catalogue {


	Catalogue::Catalogue(Minutes bus_wait_time, BusVelocity bus_velocity) : bus_wait_time_(bus_wait_time), bus_velocity_(bus_velocity) { }

	void Catalogue::SetRoutingSettings(Minutes bus_wait_time, BusVelocity bus_velocity) {
		bus_wait_time_ = bus_wait_time;
		bus_velocity_ = bus_velocity;
	}


	Catalogue::StopsPairHasher::StopsPairHasher() : N(42) {}

	size_t Catalogue::StopsPairHasher::operator()(std::pair<Stop*, Stop*> stops) const {
		size_t hash1 = hash<string>{}((*stops.first).name);
		size_t hash2 = hash<string>{}((*stops.second).name);

		return hash1 + hash2 * N;
	}



	// - добавление остановки в базу
	void Catalogue::AddStop(const Stop& stop) {
		stops_.push_back(stop);

		Stop& added_stop = stops_.back();
		stopname_to_stop_[added_stop.name] = &added_stop;
		stop_to_buses_[added_stop.name];
	}

	void Catalogue::AddStop(string_view stop_name, geo::Coordinates coordinates) {
		AddStop(Stop{ string(stop_name), coordinates });
	}


	// - добавление маршрута в базу
	void Catalogue::AddBus(std::string_view bus_name, const vector<std::string_view>& stops, bool is_ring_route) {
		Bus bus_to_add;

		bus_to_add.name = string(bus_name);
		bus_to_add.is_ring_route = is_ring_route;

		vector<Stop*> stops_on_the_route;
		Stop* prev_stop = nullptr;

		for (string_view stop : stops) {
			Stop* stop_to_add = FindStop(stop);

			stops_on_the_route.push_back(stop_to_add);

			if (prev_stop != nullptr) {
				double geo_distance_to_add = ComputeDistance((*prev_stop).coordinates, (*stop_to_add).coordinates);

				if (!distance_between_stops_.count({ prev_stop, stop_to_add })) {
					distance_between_stops_.insert({ {prev_stop, stop_to_add}, geo_distance_to_add });
				}
				if (!is_ring_route and !distance_between_stops_.count({ stop_to_add, prev_stop })) {
					distance_between_stops_.insert({ {stop_to_add, prev_stop}, geo_distance_to_add });
				}

				bus_to_add.keys_for_distance.push_back({ prev_stop, stop_to_add });
				bus_to_add.geo_distance += geo_distance_to_add;
			}

			bus_to_add.stops.insert(stop_to_add);

			prev_stop = stop_to_add;
		}
		bus_to_add.geo_distance *= is_ring_route ? 1 : 2;

		buses_.push_back(move(bus_to_add));

		Bus& added_bus = buses_.back();
		busname_to_bus_[added_bus.name] = &added_bus;

		for (Stop* stop : stops_on_the_route) {
			stop_to_buses_[stop->name].insert(added_bus.name);
		}
	}



	// - поиск остановки по имени
	Stop* Catalogue::FindStop(std::string_view stop_name) const {
		if (!stopname_to_stop_.count(stop_name)) {
			return nullptr;
		}

		return stopname_to_stop_.at(stop_name);
	}

	// - поиск маршрута по имени
	Bus* Catalogue::FindBus(string_view bus_name) const {
		if (!busname_to_bus_.count(bus_name)) {
			return nullptr;
		}

		return busname_to_bus_.at(bus_name);
	}

	// - получение информации о маршруте
	std::optional<BusInfo> Catalogue::GetBusInfo(std::string_view bus_name) const {
		BusInfo result;

		Bus* bus_ptr = FindBus(bus_name);
		if (bus_ptr == nullptr) {
			return nullopt;
		}

		Bus& bus = *bus_ptr;

		result.name = bus.name;
		result.stops_count = bus.is_ring_route ? (bus.keys_for_distance.size() + 1) : ((bus.keys_for_distance.size() + 1) * 2) - 1;
		result.unique_stops_count = bus.stops.size();

		for (const auto& key : bus.keys_for_distance) {
			result.route_length += distance_between_stops_.at(key);
			result.route_length += bus.is_ring_route ? 0 : distance_between_stops_.at({ key.second, key.first });
		}

		result.curvature = result.route_length / bus.geo_distance;

		return result;
	}


	//добавление дистанции в базу
	void Catalogue::AddDistance(Stop* stop, const unordered_map<std::string_view, double>& distanses) {
		for (const auto& [stop_name, distance] : distanses) {
			Stop* stop_dis = FindStop(stop_name);

			distance_between_stops_[{stop, stop_dis}] = distance;

			if (!distance_between_stops_.count({ stop_dis, stop })) {
				distance_between_stops_[{stop_dis, stop}] = distance;
			}
		}
	}

	double Catalogue::GetDistance(std::pair<Stop*, Stop*> stops) {
		return distance_between_stops_.at(stops);
	}
	double Catalogue::GetDistance(Stop* stop1, Stop* stop2) {
		return GetDistance({ stop1, stop2 });
	}


	//получение информации об остановке
	std::optional<StopsBuses> Catalogue::GetBusesByStop(std::string_view stop_name) const {
		if (!stop_to_buses_.count(stop_name)) {
			return nullopt;
		}

		return stop_to_buses_.at(stop_name);
	}



	size_t Catalogue::GetStopsCount() const {
		return stops_.size();
	}

	size_t Catalogue::GetBusesCount() const {
		return buses_.size();
	}

	const std::deque<Bus>& Catalogue::GetBuses() const {
		return buses_;
	}

	const std::deque<Stop>& Catalogue::GetStops() const {
		return stops_;
	}

	const std::unordered_map<std::pair<Stop*, Stop*>, double, Catalogue::StopsPairHasher>& Catalogue::GetDistances() const {
		return distance_between_stops_;
	}


	Minutes Catalogue::GetWaitTime() const {
		return bus_wait_time_;
	}

	BusVelocity Catalogue::GetBusVelocity() const {
		return bus_velocity_;
	}

	Minutes Catalogue::GetBusRideTime(std::pair<Stop*, Stop*> stops) const {
		return HoursToMinutes(KilometresToMetres(distance_between_stops_.at(stops)) / bus_velocity_);
	}

	Minutes Catalogue::GetBusRideTime(Stop* stop1, Stop* stop2) const {
		return GetBusRideTime(std::pair{ stop1, stop2 });
	}


	double Catalogue::KilometresToMetres(double length_in_metres) {
		return length_in_metres / 1000.;
	}

	Minutes Catalogue::HoursToMinutes(double time_in_hours) {
		return time_in_hours * 60.;
	}

}