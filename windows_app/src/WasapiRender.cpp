#include "WasapiRender.h"
#include <iostream>

#pragma comment(lib, "ole32.lib")

namespace audiostream {
namespace windows {

// Quick macro for COM HRESULT checks
#define CHECK_HR(hr, msg) if (FAILED(hr)) { std::cerr << msg << " HR: " << std::hex << hr << std::endl; return false; }

WasapiRender::WasapiRender() : m_audioQueue(48000), m_running(false) {
    CoInitialize(nullptr);
}

WasapiRender::~WasapiRender() {
    Stop();
    if (m_renderClient) m_renderClient->Release();
    if (m_audioClient) m_audioClient->Release();
    if (m_device) m_device->Release();
    if (m_eventHandle) CloseHandle(m_eventHandle);
    CoUninitialize();
}

bool WasapiRender::Initialize() {
    IMMDeviceEnumerator* enumerator = nullptr;
    HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**)&enumerator);
    CHECK_HR(hr, "CoCreateInstance failed.");

    // Get default render device (speakers)
    hr = enumerator->GetDefaultAudioEndpoint(eRender, eConsole, &m_device);
    enumerator->Release();
    CHECK_HR(hr, "Failed to get default audio endpoint.");

    hr = m_device->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr, (void**)&m_audioClient);
    CHECK_HR(hr, "Failed to activate IAudioClient.");

    // Setup format: 48kHz, 16-bit, Mono
    WAVEFORMATEX waveFormat = {};
    waveFormat.wFormatTag = WAVE_FORMAT_PCM;
    waveFormat.nChannels = 1;
    waveFormat.nSamplesPerSec = 48000;
    waveFormat.wBitsPerSample = 16;
    waveFormat.nBlockAlign = (waveFormat.nChannels * waveFormat.wBitsPerSample) / 8;
    waveFormat.nAvgBytesPerSec = waveFormat.nSamplesPerSec * waveFormat.nBlockAlign;

    // Use Shared Mode for now to avoid exclusive mode driver conflicts during early testing
    // AUDCLNT_STREAMFLAGS_EVENTCALLBACK is necessary for low latency event-driven rendering
    hr = m_audioClient->Initialize(AUDCLNT_SHAREMODE_SHARED, AUDCLNT_STREAMFLAGS_EVENTCALLBACK, 0, 0, &waveFormat, nullptr);
    CHECK_HR(hr, "Failed to initialize IAudioClient.");

    m_eventHandle = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    hr = m_audioClient->SetEventHandle(m_eventHandle);
    CHECK_HR(hr, "Failed to set event handle.");

    hr = m_audioClient->GetService(__uuidof(IAudioRenderClient), (void**)&m_renderClient);
    CHECK_HR(hr, "Failed to get IAudioRenderClient.");

    return true;
}

bool WasapiRender::PushAudioData(const int16_t* data, size_t numFrames) {
    // CLOCK DRIFT & JITTER MITIGATION
    // If the Android clock is running faster than the Windows clock, or the network 
    // delivers a massive burst of packets, the queue will grow and latency will increase.
    // To strictly guarantee sub-100ms latency, we drop the *oldest* audio samples
    // when the buffer exceeds our latency budget (4800 samples = 100ms).
    const size_t MAX_LATENCY_SAMPLES = 4800;
    
    while (m_audioQueue.size() + numFrames > MAX_LATENCY_SAMPLES) {
        int16_t trash;
        m_audioQueue.pop(trash);
    }

    for (size_t i = 0; i < numFrames; ++i) {
        if (!m_audioQueue.push(data[i])) break;
    }
    return true;
}

void WasapiRender::Start() {
    if (m_running) return;
    m_running = true;
    m_audioClient->Start();
    m_renderThread = std::thread(&WasapiRender::RenderLoop, this);
}

void WasapiRender::Stop() {
    if (!m_running) return;
    m_running = false;
    m_audioClient->Stop();
    SetEvent(m_eventHandle); // Wake up thread to exit
    if (m_renderThread.joinable()) {
        m_renderThread.join();
    }
}

void WasapiRender::RenderLoop() {
    // Elevate thread priority for pro audio
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);

    UINT32 bufferFrameCount;
    m_audioClient->GetBufferSize(&bufferFrameCount);

    while (m_running) {
        // Wait for OS to signal that it needs more audio
        WaitForSingleObject(m_eventHandle, INFINITE);
        if (!m_running) break;

        UINT32 numFramesPadding;
        m_audioClient->GetCurrentPadding(&numFramesPadding);
        UINT32 numFramesAvailable = bufferFrameCount - numFramesPadding;

        if (numFramesAvailable > 0) {
            BYTE* pData = nullptr;
            HRESULT hr = m_renderClient->GetBuffer(numFramesAvailable, &pData);
            if (SUCCEEDED(hr)) {
                int16_t* pcmData = reinterpret_cast<int16_t*>(pData);
                int16_t sample;
                for (UINT32 i = 0; i < numFramesAvailable; ++i) {
                    if (m_audioQueue.pop(sample)) {
                        pcmData[i] = sample;
                    } else {
                        pcmData[i] = 0; // Silence if network buffer is starved
                    }
                }
                m_renderClient->ReleaseBuffer(numFramesAvailable, 0);
            }
        }
    }
}

} // namespace windows
} // namespace audiostream
