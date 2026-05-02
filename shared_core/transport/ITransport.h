#pragma once

#include <vector>
#include <functional>
#include <cstdint>

namespace audiostream {
namespace transport {

class ITransport {
public:
    virtual ~ITransport() = default;

    // Initialize the transport layer (bind sockets, etc.)
    virtual bool Initialize() = 0;

    // Send raw packet data
    virtual bool Send(const std::vector<uint8_t>& data) = 0;

    // Callback for when data is received from the remote endpoint
    using ReceiveCallback = std::function<void(const std::vector<uint8_t>&)>;
    virtual void SetReceiveCallback(ReceiveCallback callback) = 0;

    // Stop and cleanup the transport
    virtual void Stop() = 0;
};

} // namespace transport
} // namespace audiostream
