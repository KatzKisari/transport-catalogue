#include <fstream>
#include <iostream>
#include <string_view>

#include "transport_catalogue.h"
#include "json_reader.h"


using namespace std::literals;

void PrintUsage(std::ostream& stream = std::cerr) {
    stream << "Usage: transport_catalogue [make_base|process_requests]\n"sv;
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        PrintUsage();
        return 1;
    }
    

    const std::string_view mode(argv[1]);

    if (mode == "make_base"sv) {
        transport_catalogue::JSONReader reader(std::cin);
    }
    else if (mode == "process_requests"sv) {
        transport_catalogue::JSONReader reader(std::cin, std::cout);
        reader.PrintResponse();
    }
    else {
        PrintUsage();
        return 1;
    }
}