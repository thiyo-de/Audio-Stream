#pragma once

#include "ITransport.h"
#include <string>
#include <thread>
#include <atomic>

// Note: For actual socket implementation across platforms,
// we will use standard BSD/POSIX sockets on Android,
// and WinSock on Windows. We hide the platform-specifics in the cpp file.

namespace audiostream {
namespace transport {

class TcpUsbTransport : public ITransport {
public:
    // isServer: true for Windows (listens on port), false for Android (connects to localhost)
    TcpUsbTransport(bool isServer, const std::string& address, int port);
    ~TcpUsbTransport() override;

    bool Initialize() override;
    bool Send(const std::vector<uint8_t>& data) override;
    void SetReceiveCallback(ReceiveCallback callback) override;
    void Stop() override;

private:
    void ReceiveLoop();

    bool m_isServer;
    std::string m_address;
    int m_port;

#ifdef _WIN32
    uint64_t m_socket; // SOCKET type on Windows
#else
    int m_socket;      // int type on POSIX
#endif

    std::atomic<bool> m_running;
    std::thread m_receiveThread;
    ReceiveCallback m_receiveCallback;
};

} // namespace transport
} // namespace audiostream
