#include "json_reader.h"
#include "json_builder.h"
#include "geo.h"

#include "transport_catalogue.pb.h"

#include <utility>
#include <sstream>
#include <fstream>
#include <optional>


namespace transport_catalogue {

    JSONReader::JSONReader(std::istream& input, std::ostream& output, Catalogue& catalogue)
        : mode_(ReaderMode::DEFAULT)
        , queries_(json::Load(input).GetRoot().AsDict())
        , output_(&output)
        , catalogue_(&catalogue)
        , router_(FillCatalogue())
        , renderer_(*catalogue_)
    {
        if (queries_.count("render_settings")) {
            SetRenderSettings(queries_.at("render_settings").AsDict());
        }
    }

    JSONReader::JSONReader(std::istream& input)
        : mode_(ReaderMode::SERIALIZATION)
        , queries_(json::Load(input).GetRoot().AsDict())
        , router_(FillCatalogue())
    {
        if (queries_.count("render_settings")) {
            SetRenderSettings(queries_.at("render_settings").AsDict());
        }

        transport_catalogue_serialize::Serialize(GetSerializationFilePath(), *catalogue_, renderer_, router_);
    }


    JSONReader::JSONReader(std::istream& input, std::ostream& output)
        : mode_(ReaderMode::DESERIALIZATION)
        , queries_(json::Load(input).GetRoot().AsDict())
        , deserialization_result_(transport_catalogue_serialize::Deserialize(GetSerializationFilePath()).value())
        , output_(&output)
        , catalogue_(new Catalogue(transport_catalogue_serialize::details::ConvertRawCatalogueToNormal(deserialization_result_)))
        , graph_(transport_catalogue_serialize::details::ConvertRawGraphToNormal(deserialization_result_.router().graph(), *catalogue_))
        , router_(transport_catalogue_serialize::details::ConvertRawTransportRouterToNormal(deserialization_result_.router(), *catalogue_, *graph_))
        , renderer_(*catalogue_, transport_catalogue_serialize::details::ConvertRawRenderSettingsToNormal(deserialization_result_.render_settings()))
    { }




    void JSONReader::PrintResponse() {
        using namespace std::literals;

        if (mode_ == ReaderMode::DEFAULT or mode_ == ReaderMode::DESERIALIZATION) {
            ParseStatRequests(queries_.at("stat_requests"s).AsArray());
        }
    }





    Catalogue& JSONReader::FillCatalogue() {
        using namespace std::literals;

        const json::Array& base_requests = queries_.at("base_requests").AsArray();


        if (!catalogue_)
            catalogue_ = new Catalogue();

        if (queries_.count("routing_settings")) {
            const json::Dict& routing_settings = queries_.at("routing_settings").AsDict();
            catalogue_->SetRoutingSettings(routing_settings.at("bus_wait_time"s).AsInt(), routing_settings.at("bus_velocity"s).AsDouble());
        }


        std::vector<StopQuery> stop_queries;
        std::vector<BusQuery> bus_queries;


        // * parsing base requests
        for (const json::Node& node : base_requests) {
            const json::Dict& query = node.AsDict();


            if (query.at("type"s) == "Stop"s) {
                StopQuery stop_query{ Stop{
                                            query.at("name"s).AsString(),
                                            geo::Coordinates{
                                                query.at("latitude"s).AsDouble(),
                                                query.at("longitude"s).AsDouble()} }, {} };

                if (query.count("road_distances"s)) {
                    for (const auto& [stop_name, distance] : query.at("road_distances"s).AsDict())
                        stop_query.distanses[stop_name] = distance.AsDouble();
                }

                stop_queries.push_back(std::move(stop_query));
            }

            else if (query.at("type"s) == "Bus"s) {
                BusQuery bus_query{ query.at("name"s).AsString(), {}, query.at("is_roundtrip"s).AsBool() };

                for (const json::Node& stop : query.at("stops"s).AsArray()) {
                    bus_query.stops.push_back(stop.AsString());
                }

                bus_queries.push_back(std::move(bus_query));
            }

            else {
                throw std::invalid_argument("Unknown request type"s);
            }


        }





        // * filling catalogue
        //stops
        for (const StopQuery& stop_query : stop_queries) {
            catalogue_->AddStop(stop_query.stop);
        }

        //distances
        for (const StopQuery& stop_query : stop_queries) {
            Stop* stop = catalogue_->FindStop(stop_query.stop.name);
            catalogue_->AddDistance(stop, stop_query.distanses);
        }

        //buses
        for (const BusQuery& bus_query : bus_queries) {
            catalogue_->AddBus(bus_query.bus_name, bus_query.stops, bus_query.is_ring_route);
        }



        return *catalogue_;
    }





    void JSONReader::SetRenderSettings(const json::Dict& render_settings) {
        using namespace std::literals;

        renderer_
            .SetWidth(render_settings.at("width"s).AsDouble())
            .SetHeight(render_settings.at("height"s).AsDouble())

            .SetPadding(render_settings.at("padding"s).AsDouble())

            .SetLineWidth(render_settings.at("line_width"s).AsDouble())
            .SetStopRadius(render_settings.at("stop_radius"s).AsDouble())

            .SetBusLabelFontSize(render_settings.at("bus_label_font_size"s).AsInt())
            .SetStopLabelFontSize(render_settings.at("stop_label_font_size"s).AsInt())

            .SetUnderlayerWidth(render_settings.at("underlayer_width"s).AsDouble())
            .SetUnderlayerColor(ColorFromNode(render_settings.at("underlayer_color"s)));


        const json::Array& bus_label_offset = render_settings.at("bus_label_offset"s).AsArray();

        if (bus_label_offset.size() != 2)  throw std::invalid_argument("Invalid bus label offset");
        double dx = bus_label_offset.front().AsDouble();
        double dy = bus_label_offset.back().AsDouble();

        renderer_.SetBusLabelOffset(dx, dy);


        const json::Array& stop_label_offset = render_settings.at("stop_label_offset"s).AsArray();

        if (stop_label_offset.size() != 2)  throw std::invalid_argument("Invalid stop label offset");
        dx = stop_label_offset.front().AsDouble();
        dy = stop_label_offset.back().AsDouble();

        renderer_.SetStopLabelOffset(dx, dy);



        std::vector<svg::Color> palette;

        for (const json::Node& node : render_settings.at("color_palette"s).AsArray()) {
            palette.push_back(ColorFromNode(node));
        }

        renderer_.SetColorPalette(std::move(palette));

    }






    void JSONReader::ParseStatRequests(const json::Array& base_requests) {
        using namespace std::literals;

        json::Array full_response;

        for (const json::Node& node : base_requests) {
            const json::Dict& request = node.AsDict();

            json::Dict response;

            if (request.at("type"s) == "Stop"s) {
                if (std::optional<StopsBuses> stop_info = catalogue_->GetBusesByStop(request.at("name"s).AsString())) {
                    json::Array buses;
                    for (std::string_view bus : *stop_info) {
                        buses.push_back(std::string(bus));
                    }
                    response["buses"s] = buses;
                }
                else {
                    response["error_message"s] = "not found"s;
                }
            }
            else if (request.at("type"s) == "Bus"s) {
                if (std::optional<BusInfo> bus_info = catalogue_->GetBusInfo(request.at("name"s).AsString())) {
                    response = json::Builder{}.StartDict()
                        .Key("curvature"s).Value(bus_info->curvature)
                        .Key("route_length"s).Value(bus_info->route_length)
                        .Key("stop_count"s).Value(static_cast<int>(bus_info->stops_count))
                        .Key("unique_stop_count"s).Value(static_cast<int>(bus_info->unique_stops_count)).
                        EndDict().Build().AsDict();
                }
                else {
                    response["error_message"s] = "not found"s;
                }
            }
            else if (request.at("type"s) == "Map"s) {
                std::ostringstream map_output;

                renderer_.Render(map_output);

                response["map"s] = map_output.str();
            }
            else if (request.at("type"s) == "Route"s) {
                response = router_.BuildRoute(request).AsDict();
            }
            else {
                throw std::invalid_argument("Unknown request type");
            }


            response["request_id"s] = request.at("id"s);

            full_response.push_back(response);
        }

        json::Print(json::Document(full_response), *output_);
    }

    renderer::MapRenderer& JSONReader::FillRenderer() {
        for (const Bus& bus : catalogue_->GetBuses()) {
            renderer_.AddBus(bus);
        }

        return renderer_;
    }





    svg::Color JSONReader::ColorFromNode(const json::Node& node) {
        if (node.IsString()) {
            return node.AsString();
        }
        else if (node.IsArray()) {
            const json::Array& color = node.AsArray();

            switch (color.size()) {
            case (3):
                return svg::Rgb(color.at(0).AsInt(),
                    color.at(1).AsInt(),
                    color.at(2).AsInt());

            case (4):
                return svg::Rgba(color.at(0).AsInt(),
                    color.at(1).AsInt(),
                    color.at(2).AsInt(),
                    color.at(3).AsDouble());

            default:
                throw std::invalid_argument("Invalid color input");
            }
        }
        else {
            throw std::invalid_argument("Invalid color input");
        }
    }

    std::filesystem::path JSONReader::GetSerializationFilePath() const {
        return std::filesystem::path(queries_.at("serialization_settings").AsDict().at("file").AsString());
    }
}