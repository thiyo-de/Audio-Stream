#pragma once

#include "ITransport.h"
#include <string>
#include <atomic>
#include <thread>

#ifdef _WIN32
#include <winsock2.h>
#else
#include <netinet/in.h>
#endif

namespace audiostream {
namespace transport {

class UdpTransport : public ITransport {
public:
    UdpTransport(bool isServer, const std::string& ip, int port);
    ~UdpTransport() override;

    bool Initialize() override;
    void Stop() override;
    bool Send(const std::vector<uint8_t>& data) override;
    void SetReceiveCallback(std::function<void(const std::vector<uint8_t>&)> callback) override;

private:
    void ReceiveLoop();

    bool m_isServer;
    std::string m_ip;
    int m_port;

#ifdef _WIN32
    SOCKET m_socket;
#else
    int m_socket;
#endif

    struct sockaddr_in m_remoteAddr;
    bool m_hasRemoteAddr; // True for client always, True for server once it receives a packet

    std::atomic<bool> m_running;
    std::thread m_receiveThread;
    std::function<void(const std::vector<uint8_t>&)> m_receiveCallback;
};

} // namespace transport
} // namespace audiostream
