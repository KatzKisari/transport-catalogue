cmake_minimum_required(VERSION 3.10)

project(transport_catalogue CXX)
set(CMAKE_CXX_STANDARD 17)

find_package(Protobuf REQUIRED)
find_package(Threads REQUIRED)

protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS graph.proto transport_router.proto svg.proto map_renderer.proto transport_catalogue.proto)
string(REPLACE "protobuf.lib" "protobufd.lib" "Protobuf_LIBRARY_DEBUG" "${Protobuf_LIBRARY_DEBUG}")
string(REPLACE "protobuf.a" "protobufd.a" "Protobuf_LIBRARY_DEBUG" "${Protobuf_LIBRARY_DEBUG}")


set(BASE_FILES transport_catalogue.cpp transport_catalogue.h geo.cpp geo.h domain.cpp domain.h)
set(JSON json.cpp json.h json_builder.cpp json_builder.h json_reader.cpp json_reader.h)
set(ROUTER ranges.h graph.h router.h transport_router.cpp transport_router.h)
set(MAP_RENDERER map_renderer.cpp map_renderer.h svg.h svg.cpp)
set(SERIALIZATION ${PROTO_SRCS} ${PROTO_HDRS} map_renderer.pb.h map_renderer.pb.cc transport_catalogue.pb.h transport_catalogue.pb.cc serialization.h serialization.cpp)

set(TRANSPORT_CATALOGUE_FILES 
	${BASE_FILES}
	${JSON}
	${ROUTER}
	${MAP_RENDERER}
	${SERIALIZATION}
)

add_executable(transport_catalogue ${TRANSPORT_CATALOGUE_FILES} main.cpp)

target_include_directories(transport_catalogue PUBLIC ${Protobuf_INCLUDE_DIRS})
target_include_directories(transport_catalogue PUBLIC ${CMAKE_CURRENT_BINARY_DIR})

target_link_libraries(transport_catalogue ${Protobuf_LIBRARY} Threads::Threads)
target_link_libraries(transport_catalogue "$<IF:$<CONFIG:Debug>,${Protobuf_LIBRARY_DEBUG},${Protobuf_LIBRARY}>" Threads::Threads)