#include <iostream>
#include <vector>
#include <string>
#include <thread>
#include <chrono>
#include "TcpUsbTransport.h"

int main() {
    std::cout << "=======================================" << std::endl;
    std::cout << "      AudioStream Host (Windows)       " << std::endl;
    std::cout << "=======================================" << std::endl;

    // Initialize the transport in Server mode (true)
    // We bind to 0.0.0.0 (all interfaces) on port 5000.
    // The ADB tunnel will forward Android traffic to this port on localhost.
    audiostream::transport::TcpUsbTransport transport(true, "0.0.0.0", 5000);

    // Register a callback to handle incoming packets from the Android app
    transport.SetReceiveCallback([](const std::vector<uint8_t>& data) {
        // For Phase 1, we just print out the raw string data received to prove the tunnel works.
        std::string text(data.begin(), data.end());
        std::cout << "[Android] -> " << text << std::endl;
    });

    // Start listening
    if (!transport.Initialize()) {
        std::cerr << "CRITICAL ERROR: Failed to initialize transport layer." << std::endl;
        return 1;
    }

    std::cout << "\nHost is actively running." << std::endl;
    std::cout << "Run 'windows_app/scripts/setup_adb_tunnel.bat' to ensure the USB bridge is open." << std::endl;
    std::cout << "Type something and hit Enter to send a test message to Android, or type 'exit' to quit." << std::endl;
    
    // Main loop: keep the application alive and allow sending test messages
    while (true) {
        std::string input;
        std::getline(std::cin, input);
        
        if (input == "exit") {
            break;
        }

        if (!input.empty()) {
            std::vector<uint8_t> payload(input.begin(), input.end());
            if (transport.Send(payload)) {
                std::cout << "[Windows] -> " << input << std::endl;
            } else {
                std::cerr << "Failed to send message. Is Android connected?" << std::endl;
            }
        }
    }

    std::cout << "Shutting down..." << std::endl;
    transport.Stop();
    return 0;
}
