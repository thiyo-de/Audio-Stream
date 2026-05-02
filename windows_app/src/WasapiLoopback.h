#pragma once

#include <windows.h>
#include <mmdeviceapi.h>
#include <audioclient.h>
#include <vector>
#include <thread>
#include <atomic>
#include "LockFreeQueue.h"

namespace audiostream {
namespace windows {

class WasapiLoopback {
public:
    WasapiLoopback();
    ~WasapiLoopback();

    bool Initialize();
    void Start();
    void Stop();

    // Pulls exactly exactSamples from the capture queue
    bool PopAudioFrame(std::vector<int16_t>& outData, int exactSamples);

private:
    void CaptureLoop();

    IMMDevice* m_device = nullptr;
    IAudioClient* m_audioClient = nullptr;
    IAudioCaptureClient* m_captureClient = nullptr;
    HANDLE m_eventHandle = nullptr;

    std::atomic<bool> m_running;
    std::thread m_captureThread;
    
    // 48000Hz, 1 Channel, 16-bit PCM buffer
    LockFreeQueue<int16_t> m_audioQueue;
};

} // namespace windows
} // namespace audiostream
