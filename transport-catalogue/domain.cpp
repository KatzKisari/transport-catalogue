#include "domain.h"


namespace transport_catalogue {

	bool Stop::operator==(const Stop& rhs) const {
		return name == rhs.name;
	}

	std::vector<Stop*> Bus::GetStops() const {
		std::vector<Stop*> result;
		result.reserve(keys_for_distance.size() + 1);

		result.push_back(keys_for_distance.front().first);

		for (auto [stop0, stop1] : keys_for_distance) {
			result.push_back(stop1);
		}


		return result;
	}

	bool Bus::operator==(const Bus& rhs) const {
		return name == rhs.name;
	}


	bool Bus::operator < (const Bus& rhs) const { 
		return name < rhs.name;
	}
	bool Bus::operator > (const Bus& rhs) const { 
		return name > rhs.name; 
	}

	bool Bus::operator <= (const Bus & rhs) const { 
		return name <= rhs.name; 
	}
	bool Bus::operator>=(const Bus& rhs) const { 
		return name >= rhs.name; 
	}

}