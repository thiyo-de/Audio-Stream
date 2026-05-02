#include "WasapiLoopback.h"
#include <iostream>

#pragma comment(lib, "ole32.lib")

namespace audiostream {
namespace windows {

#define CHECK_HR(hr, msg) if (FAILED(hr)) { std::cerr << msg << " HR: " << std::hex << hr << std::endl; return false; }

WasapiLoopback::WasapiLoopback() : m_audioQueue(48000), m_running(false) {
    CoInitialize(nullptr);
}

WasapiLoopback::~WasapiLoopback() {
    Stop();
    if (m_captureClient) m_captureClient->Release();
    if (m_audioClient) m_audioClient->Release();
    if (m_device) m_device->Release();
    if (m_eventHandle) CloseHandle(m_eventHandle);
    CoUninitialize();
}

bool WasapiLoopback::Initialize() {
    IMMDeviceEnumerator* enumerator = nullptr;
    HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**)&enumerator);
    CHECK_HR(hr, "CoCreateInstance failed.");

    // Loopback captures what is sent to the DEFAULT RENDER device (speakers)
    hr = enumerator->GetDefaultAudioEndpoint(eRender, eConsole, &m_device);
    enumerator->Release();
    CHECK_HR(hr, "Failed to get default audio endpoint for loopback.");

    hr = m_device->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr, (void**)&m_audioClient);
    CHECK_HR(hr, "Failed to activate IAudioClient.");

    WAVEFORMATEX waveFormat = {};
    waveFormat.wFormatTag = WAVE_FORMAT_PCM;
    waveFormat.nChannels = 1; // Downmix to Mono for Opus Voice
    waveFormat.nSamplesPerSec = 48000;
    waveFormat.wBitsPerSample = 16;
    waveFormat.nBlockAlign = (waveFormat.nChannels * waveFormat.wBitsPerSample) / 8;
    waveFormat.nAvgBytesPerSec = waveFormat.nSamplesPerSec * waveFormat.nBlockAlign;

    // We use AUTOCONVERTPCM to force Windows to give us exactly 48kHz Mono 16-bit, regardless of the user's sound card setting
    DWORD flags = AUDCLNT_STREAMFLAGS_LOOPBACK | AUDCLNT_STREAMFLAGS_EVENTCALLBACK | AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM | AUDCLNT_STREAMFLAGS_SRC_DEFAULT_QUALITY;
    hr = m_audioClient->Initialize(AUDCLNT_SHAREMODE_SHARED, flags, 0, 0, &waveFormat, nullptr);
    if (FAILED(hr)) {
        // Fallback to polling mode if event mode loopback fails
        flags = AUDCLNT_STREAMFLAGS_LOOPBACK | AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM | AUDCLNT_STREAMFLAGS_SRC_DEFAULT_QUALITY;
        hr = m_audioClient->Initialize(AUDCLNT_SHAREMODE_SHARED, flags, 10000000, 0, &waveFormat, nullptr);
        CHECK_HR(hr, "Failed to initialize loopback IAudioClient.");
    } else {
        m_eventHandle = CreateEvent(nullptr, FALSE, FALSE, nullptr);
        m_audioClient->SetEventHandle(m_eventHandle);
    }

    hr = m_audioClient->GetService(__uuidof(IAudioCaptureClient), (void**)&m_captureClient);
    CHECK_HR(hr, "Failed to get IAudioCaptureClient.");

    return true;
}

bool WasapiLoopback::PopAudioFrame(std::vector<int16_t>& outData, int exactSamples) {
    if (m_audioQueue.size() < exactSamples) return false;
    int16_t sample;
    for (int i = 0; i < exactSamples; ++i) {
        if (m_audioQueue.pop(sample)) outData.push_back(sample);
    }
    return true;
}

void WasapiLoopback::Start() {
    if (m_running) return;
    m_running = true;
    m_audioClient->Start();
    m_captureThread = std::thread(&WasapiLoopback::CaptureLoop, this);
}

void WasapiLoopback::Stop() {
    if (!m_running) return;
    m_running = false;
    m_audioClient->Stop();
    if (m_eventHandle) SetEvent(m_eventHandle);
    if (m_captureThread.joinable()) m_captureThread.join();
}

void WasapiLoopback::CaptureLoop() {
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);

    while (m_running) {
        if (m_eventHandle) {
            WaitForSingleObject(m_eventHandle, 10); // Wait up to 10ms
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(5)); // Polling fallback
        }
        
        if (!m_running) break;

        UINT32 packetLength = 0;
        HRESULT hr = m_captureClient->GetNextPacketSize(&packetLength);
        
        while (packetLength != 0) {
            BYTE* pData;
            UINT32 numFramesAvailable;
            DWORD flags;
            
            hr = m_captureClient->GetBuffer(&pData, &numFramesAvailable, &flags, nullptr, nullptr);
            if (SUCCEEDED(hr)) {
                int16_t* pcmData = reinterpret_cast<int16_t*>(pData);
                for (UINT32 i = 0; i < numFramesAvailable; ++i) {
                    int16_t sample = (flags & AUDCLNT_BUFFERFLAGS_SILENT) ? 0 : pcmData[i];
                    m_audioQueue.push(sample);
                }
                m_captureClient->ReleaseBuffer(numFramesAvailable);
            }
            hr = m_captureClient->GetNextPacketSize(&packetLength);
        }
    }
}

} // namespace windows
} // namespace audiostream
