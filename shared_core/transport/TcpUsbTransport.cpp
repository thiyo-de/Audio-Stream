#include "TcpUsbTransport.h"
#include <iostream>

#ifdef _WIN32
#include <winsock2.h>
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

TcpUsbTransport::TcpUsbTransport(bool isServer, const std::string& address, int port)
    : m_isServer(isServer)
    , m_address(address)
    , m_port(port)
    , m_socket(INVALID_SOCKET)
    , m_running(false)
{
#ifdef _WIN32
    // Initialize Winsock on Windows
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif
}

TcpUsbTransport::~TcpUsbTransport() {
    Stop();
#ifdef _WIN32
    WSACleanup();
#endif
}

bool TcpUsbTransport::Initialize() {
    m_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (m_socket == INVALID_SOCKET) {
        std::cerr << "Failed to create socket." << std::endl;
        return false;
    }

    // Set TCP_NODELAY to reduce latency (disable Nagle's algorithm)
    int flag = 1;
    setsockopt(m_socket, IPPROTO_TCP, TCP_NODELAY, (char*)&flag, sizeof(int));

    sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(m_port);
    inet_pton(AF_INET, m_address.c_str(), &addr.sin_addr);

    if (m_isServer) {
        // Windows acts as the server, binding to the local port forwarded by ADB
        if (bind(m_socket, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
            std::cerr << "Bind failed." << std::endl;
            closesocket(m_socket);
            return false;
        }

        if (listen(m_socket, 1) == SOCKET_ERROR) {
            std::cerr << "Listen failed." << std::endl;
            closesocket(m_socket);
            return false;
        }

        std::cout << "Waiting for Android client to connect via ADB tunnel..." << std::endl;
        
        sockaddr_in clientAddr;
        socklen_t clientLen = sizeof(clientAddr);
        auto clientSocket = accept(m_socket, (sockaddr*)&clientAddr, &clientLen);
        if (clientSocket == INVALID_SOCKET) {
            std::cerr << "Accept failed." << std::endl;
            return false;
        }

        // We close the listener and just keep the client socket
        closesocket(m_socket);
        m_socket = clientSocket;
        
        // Ensure TCP_NODELAY is also set on the accepted client socket
        setsockopt(m_socket, IPPROTO_TCP, TCP_NODELAY, (char*)&flag, sizeof(int));
        std::cout << "Client connected!" << std::endl;
    } else {
        // Android acts as the client, connecting to the localhost port forwarded by ADB
        if (connect(m_socket, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
            std::cerr << "Connect failed." << std::endl;
            closesocket(m_socket);
            return false;
        }
        std::cout << "Connected to Windows host!" << std::endl;
    }

    m_running = true;
    m_receiveThread = std::thread(&TcpUsbTransport::ReceiveLoop, this);
    return true;
}

bool TcpUsbTransport::Send(const std::vector<uint8_t>& data) {
    if (!m_running || m_socket == INVALID_SOCKET) return false;

    // Send length prefix (4 bytes) to ensure we decode the full packet on the other side
    uint32_t len = htonl(data.size());
    if (send(m_socket, (const char*)&len, sizeof(len), 0) == SOCKET_ERROR) return false;

    // Send actual payload
    if (send(m_socket, (const char*)data.data(), data.size(), 0) == SOCKET_ERROR) return false;

    return true;
}

void TcpUsbTransport::SetReceiveCallback(ReceiveCallback callback) {
    m_receiveCallback = callback;
}

void TcpUsbTransport::Stop() {
    if (m_running) {
        m_running = false;
        closesocket(m_socket);
        m_socket = INVALID_SOCKET;
        if (m_receiveThread.joinable()) {
            m_receiveThread.join();
        }
    }
}

void TcpUsbTransport::ReceiveLoop() {
    while (m_running) {
        uint32_t len_net = 0;
        
        // Wait for the 4-byte length prefix
        int bytes = recv(m_socket, (char*)&len_net, sizeof(len_net), 0);
        if (bytes <= 0) break; // Disconnected or error

        uint32_t len = ntohl(len_net);
        std::vector<uint8_t> buffer(len);

        // Read the exact payload length
        uint32_t totalReceived = 0;
        while (totalReceived < len) {
            bytes = recv(m_socket, (char*)(buffer.data() + totalReceived), len - totalReceived, 0);
            if (bytes <= 0) break;
            totalReceived += bytes;
        }

        // Fire callback to the Audio Engine
        if (totalReceived == len && m_receiveCallback) {
            m_receiveCallback(buffer);
        }
    }
    m_running = false;
}

} // namespace transport
} // namespace audiostream
