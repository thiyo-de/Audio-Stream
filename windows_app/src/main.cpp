#include <iostream>
#include <vector>
#include <thread>
#include <chrono>
#include "TcpUsbTransport.h"
#include "WasapiRender.h"

int main() {
    std::cout << "=======================================" << std::endl;
    std::cout << "      AudioStream Host (Windows)       " << std::endl;
    std::cout << "=======================================" << std::endl;

    // Initialize the transport in Server mode (true)
    // We bind to 0.0.0.0 (all interfaces) on port 5000.
    // The ADB tunnel will forward Android traffic to this port on localhost.
    audiostream::transport::TcpUsbTransport transport(true, "0.0.0.0", 5000);
    
    audiostream::windows::WasapiRender audioRender;
    if (!audioRender.Initialize()) {
        std::cerr << "CRITICAL ERROR: Failed to initialize WASAPI Render." << std::endl;
        return 1;
    }
    audioRender.Start();

    // Register a callback to handle incoming packets from the Android app
    transport.SetReceiveCallback([&audioRender](const std::vector<uint8_t>& data) {
        // In Phase 2, we receive raw PCM int16_t array from Oboe via network
        // Size of int16_t is 2 bytes, so numFrames = data.size() / 2
        size_t numFrames = data.size() / sizeof(int16_t);
        const int16_t* pcmData = reinterpret_cast<const int16_t*>(data.data());
        audioRender.PushAudioData(pcmData, numFrames);
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
