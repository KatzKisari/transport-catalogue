#pragma once

#include "transport_catalogue.h"
#include "map_renderer.h"
#include "transport_router.h"
#include "svg.h"

#include <iostream>
#include <optional>
#include <filesystem>

#include "transport_catalogue.pb.h"


namespace transport_catalogue_serialize {
	namespace details {

		Color ConvertColorToRaw(const svg::Color& color);
		svg::Color ConvertRawColorToNormal(const Color& raw_color);


		RenderSettings ConvertMapRendererToRaw(const transport_catalogue::renderer::MapRenderer& renderer);
		transport_catalogue::renderer::RenderSettings ConvertRawRenderSettingsToNormal(const RenderSettings& raw_settings);


		Graph ConvertGraphToRaw(const graph::DirectedWeightedGraph<transport_catalogue::TransportRouter::RouteWeight>& graph);
		graph::DirectedWeightedGraph<transport_catalogue::TransportRouter::RouteWeight> ConvertRawGraphToNormal(const Graph& raw_graph, const transport_catalogue::Catalogue& catalogue);

		Router ConvertTransportRouterToRaw(const transport_catalogue::TransportRouter& router);
		transport_catalogue::TransportRouter ConvertRawTransportRouterToNormal(
			const Router& raw_router, 
			const transport_catalogue::Catalogue& catalogue,
			graph::DirectedWeightedGraph<transport_catalogue::TransportRouter::RouteWeight>& graph);



		TransportCatalogue ConvertCatalogueToRaw(const transport_catalogue::Catalogue& catalogue, const transport_catalogue::renderer::MapRenderer& renderer, const transport_catalogue::TransportRouter& router);
		transport_catalogue::Catalogue ConvertRawCatalogueToNormal(const TransportCatalogue& catalogue);



		void SerializeRawCatalogue(std::ostream& output, const TransportCatalogue& catalogue);
	}



	void Serialize(std::ostream& output, const transport_catalogue::Catalogue& catalogue, const transport_catalogue::renderer::MapRenderer& renderer, const transport_catalogue::TransportRouter& router);
	std::optional<TransportCatalogue> Deserialize(std::istream& input);


	void Serialize(const std::filesystem::path& output_file, const transport_catalogue::Catalogue& catalogue, const transport_catalogue::renderer::MapRenderer& renderer, const transport_catalogue::TransportRouter& router);
	std::optional<TransportCatalogue> Deserialize(const std::filesystem::path& input_file);
}