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

class WasapiRender {
public:
    WasapiRender();
    ~WasapiRender();

    bool Initialize();
    void Start();
    void Stop();

    // Network thread pushes received audio here
    bool PushAudioData(const int16_t* data, size_t numFrames);

private:
    void RenderLoop();

    IMMDevice* m_device = nullptr;
    IAudioClient* m_audioClient = nullptr;
    IAudioRenderClient* m_renderClient = nullptr;
    HANDLE m_eventHandle = nullptr;

    std::atomic<bool> m_running;
    std::thread m_renderThread;
    
    // We expect 48000Hz, 1 Channel, 16-bit PCM
    LockFreeQueue<int16_t> m_audioQueue;
};

} // namespace windows
} // namespace audiostream
