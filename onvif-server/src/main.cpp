#include "app.h"

#include <exception>
#include <iostream>

int main(int argc, char* argv[]) {
    try {
        return afterveda_onvif::run(argc, argv);
    } catch (const std::exception& error) {
        std::cerr << "[error] afterveda-onvif: " << error.what() << "\n";
        return 1;
    }
}
