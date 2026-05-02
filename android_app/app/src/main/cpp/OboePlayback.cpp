#include "OboePlayback.h"
#include <android/log.h>

#define LOG_TAG "OboePlayback"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

namespace audiostream {
namespace android {

OboePlayback::OboePlayback() : m_audioQueue(48000) { 
}

OboePlayback::~OboePlayback() {
    StopPlayback();
}

bool OboePlayback::StartPlayback() {
    oboe::AudioStreamBuilder builder;
    
    // Playback on speakers/headphones
    builder.setDirection(oboe::Direction::Output)
           ->setPerformanceMode(oboe::PerformanceMode::LowLatency)
           ->setSharingMode(oboe::SharingMode::Exclusive)
           ->setFormat(oboe::AudioFormat::I16)
           ->setChannelCount(oboe::ChannelCount::Mono)
           ->setSampleRate(48000)
           ->setDataCallback(this);

    oboe::Result result = builder.openStream(m_playbackStream);
    if (result != oboe::Result::OK) {
        LOGE("Failed to create playback stream. Error: %s", oboe::convertToText(result));
        return false;
    }

    result = m_playbackStream->requestStart();
    if (result != oboe::Result::OK) {
        LOGE("Failed to start playback stream. Error: %s", oboe::convertToText(result));
        m_playbackStream->close();
        return false;
    }

    LOGI("Successfully started Oboe playback stream.");
    return true;
}

void OboePlayback::StopPlayback() {
    if (m_playbackStream) {
        m_playbackStream->requestStop();
        m_playbackStream->close();
        m_playbackStream.reset();
        LOGI("Oboe playback stream stopped.");
    }
}

bool OboePlayback::PushAudioData(const int16_t* data, size_t numFrames) {
    // CLOCK DRIFT & JITTER MITIGATION
    // If the Windows clock is running faster than the Android clock, or the network 
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

oboe::DataCallbackResult OboePlayback::onAudioReady(
    oboe::AudioStream *oboeStream, void *audioData, int32_t numFrames) 
{
    auto* pcmData = static_cast<int16_t*>(audioData);
    int32_t channels = oboeStream->getChannelCount();

    int16_t sample;
    for (int32_t i = 0; i < numFrames * channels; ++i) {
        if (m_audioQueue.pop(sample)) {
            pcmData[i] = sample;
        } else {
            pcmData[i] = 0; // Play silence if starved
        }
    }

    return oboe::DataCallbackResult::Continue;
}

} // namespace android
} // namespace audiostream
