#pragma once

#include "transport_catalogue.h"
#include "map_renderer.h"
#include "transport_router.h"
#include "domain.h"
#include "json.h"
#include "serialization.h"

#include <iostream>
#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>
#include <filesystem>


namespace transport_catalogue {

    class JSONReader {
        enum class ReaderMode {
            DEFAULT,
            SERIALIZATION,
            DESERIALIZATION
        };

    public:
        // * Default Mode
        explicit JSONReader(std::istream& input, std::ostream& output, Catalogue& catalogue);
        // * Serialization Mode
        explicit JSONReader(std::istream& input);
        // * Deserialization Mode
        explicit JSONReader(std::istream& input, std::ostream& output);

        void PrintResponse();

    private:
        const ReaderMode mode_;
        json::Dict queries_;
        transport_catalogue_serialize::TransportCatalogue deserialization_result_;
        std::ostream* output_ = nullptr;
        Catalogue* catalogue_ = nullptr;;
        std::optional<graph::DirectedWeightedGraph<TransportRouter::RouteWeight>> graph_ = std::nullopt;
        TransportRouter router_;
        renderer::MapRenderer renderer_;


        struct BusQuery {
            std::string_view bus_name;
            std::vector<std::string_view> stops;
            bool is_ring_route = false;
        };

        struct StopQuery {
            Stop stop;
            std::unordered_map<std::string_view, double> distanses;
        };

        Catalogue& FillCatalogue();
        renderer::MapRenderer& FillRenderer();
        void SetRenderSettings(const json::Dict& render_settings);
        void ParseStatRequests(const json::Array& stat_requests);



        svg::Color static ColorFromNode(const json::Node& node);

        std::filesystem::path GetSerializationFilePath() const;
    };
}