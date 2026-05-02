#include "UdpTransport.h"
#include <iostream>
#include <cstring>

#ifdef _WIN32
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#define closesocket close
#endif

namespace audiostream {
namespace transport {

UdpTransport::UdpTransport(bool isServer, const std::string& ip, int port)
    : m_isServer(isServer), m_ip(ip), m_port(port), m_socket(INVALID_SOCKET), m_hasRemoteAddr(false), m_running(false) {
    memset(&m_remoteAddr, 0, sizeof(m_remoteAddr));
}

UdpTransport::~UdpTransport() {
    Stop();
}

bool UdpTransport::Initialize() {
#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed." << std::endl;
        return false;
    }
#endif

    m_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (m_socket == INVALID_SOCKET) {
        std::cerr << "UDP Socket creation failed." << std::endl;
        return false;
    }

    struct sockaddr_in localAddr;
    memset(&localAddr, 0, sizeof(localAddr));
    localAddr.sin_family = AF_INET;
    localAddr.sin_port = htons(m_port);

    if (m_isServer) {
        // Server binds to ANY address on the given port
        localAddr.sin_addr.s_addr = INADDR_ANY;
        if (bind(m_socket, (struct sockaddr*)&localAddr, sizeof(localAddr)) == SOCKET_ERROR) {
            std::cerr << "UDP Bind failed." << std::endl;
            closesocket(m_socket);
            return false;
        }
        std::cout << "UDP Server listening on port " << m_port << std::endl;
    } else {
        // Client binds to ANY local port, targets the Server IP
        localAddr.sin_addr.s_addr = INADDR_ANY;
        localAddr.sin_port = 0; // Let OS choose local port
        if (bind(m_socket, (struct sockaddr*)&localAddr, sizeof(localAddr)) == SOCKET_ERROR) {
            std::cerr << "UDP Bind failed." << std::endl;
            closesocket(m_socket);
            return false;
        }

        m_remoteAddr.sin_family = AF_INET;
        m_remoteAddr.sin_port = htons(m_port);
        inet_pton(AF_INET, m_ip.c_str(), &m_remoteAddr.sin_addr);
        m_hasRemoteAddr = true;

        // Send a dummy packet so the server knows our IP/Port for two-way audio
        std::vector<uint8_t> punchPacket = {'H', 'O', 'L', 'E', 'P', 'U', 'N', 'C', 'H'};
        Send(punchPacket);
        std::cout << "UDP Client initialized. Target: " << m_ip << ":" << m_port << std::endl;
    }

    m_running = true;
    m_receiveThread = std::thread(&UdpTransport::ReceiveLoop, this);
    return true;
}

void UdpTransport::Stop() {
    m_running = false;
    if (m_socket != INVALID_SOCKET) {
        closesocket(m_socket);
        m_socket = INVALID_SOCKET;
    }
    if (m_receiveThread.joinable()) {
        m_receiveThread.join();
    }
#ifdef _WIN32
    WSACleanup();
#endif
}

bool UdpTransport::Send(const std::vector<uint8_t>& data) {
    if (m_socket == INVALID_SOCKET || !m_hasRemoteAddr) return false;

    int sentBytes = sendto(m_socket, reinterpret_cast<const char*>(data.data()), data.size(), 0,
                           (struct sockaddr*)&m_remoteAddr, sizeof(m_remoteAddr));
    return sentBytes != SOCKET_ERROR;
}

void UdpTransport::SetReceiveCallback(std::function<void(const std::vector<uint8_t>&)> callback) {
    m_receiveCallback = callback;
}

void UdpTransport::ReceiveLoop() {
    char buffer[8192];
    struct sockaddr_in senderAddr;
#ifdef _WIN32
    int senderAddrSize = sizeof(senderAddr);
#else
    socklen_t senderAddrSize = sizeof(senderAddr);
#endif

    while (m_running) {
        int bytesRead = recvfrom(m_socket, buffer, sizeof(buffer), 0,
                                 (struct sockaddr*)&senderAddr, &senderAddrSize);
        
        if (bytesRead > 0) {
            if (m_isServer && !m_hasRemoteAddr) {
                // First packet received on server configures the remote endpoint for Full-Duplex
                m_remoteAddr = senderAddr;
                m_hasRemoteAddr = true;
                std::cout << "UDP Server locked onto client endpoint." << std::endl;
            }

            // Ignore the HOLEPUNCH packet from client
            if (bytesRead == 9 && memcmp(buffer, "HOLEPUNCH", 9) == 0) {
                continue;
            }

            if (m_receiveCallback) {
                std::vector<uint8_t> payload(buffer, buffer + bytesRead);
                m_receiveCallback(payload);
            }
        } else if (bytesRead == 0 || (bytesRead == SOCKET_ERROR && !m_running)) {
            break;
        }
    }
}

} // namespace transport
} // namespace audiostream
