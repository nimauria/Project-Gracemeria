#include <iostream>

#include <xenon/core/runtime.hpp>

int main()
{
    std::cout << "[Gracemeria] Starting Project Gracemeria...\n";

    xenon::RuntimeConfig config{};

    // Keep everything we don't need disabled during initial bring-up.
    config.enable_logging = true;
    config.enable_graphics = false;
    config.enable_audio = false;
    config.enable_input = false;
    config.enable_network = false;

    xenon::Runtime runtime;

    if (!runtime.initialize(config))
    {
        std::cerr << "[Gracemeria] Failed to initialize Xenon runtime.\n";
        return 1;
    }

    std::cout << "[Gracemeria] Xenon runtime initialized successfully.\n";

    runtime.shutdown();

    std::cout << "[Gracemeria] Shutdown complete.\n";

    return 0;
}