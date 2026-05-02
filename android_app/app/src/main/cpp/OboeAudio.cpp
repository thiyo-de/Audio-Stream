#include "OboeAudio.h"
#include <android/log.h>

#define LOG_TAG "OboeAudio"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

namespace audiostream {
namespace android {

OboeAudio::OboeAudio() : m_audioQueue(48000) { // 1 second of buffer at 48kHz
}

OboeAudio::~OboeAudio() {
    StopCapture();
}

bool OboeAudio::StartCapture() {
    oboe::AudioStreamBuilder builder;
    
    // Request lowest latency possible
    builder.setDirection(oboe::Direction::Input)
           ->setPerformanceMode(oboe::PerformanceMode::LowLatency)
           ->setSharingMode(oboe::SharingMode::Exclusive)
           ->setFormat(oboe::AudioFormat::I16) // 16-bit PCM
           ->setChannelCount(oboe::ChannelCount::Mono) // 1 Channel (Mic)
           ->setSampleRate(48000) // Standard for VoIP
           ->setInputPreset(oboe::InputPreset::VoiceCommunication) // Hardware AEC
           ->setDataCallback(this);

    oboe::Result result = builder.openStream(m_recordingStream);
    if (result != oboe::Result::OK) {
        LOGE("Failed to create recording stream. Error: %s", oboe::convertToText(result));
        return false;
    }

    result = m_recordingStream->requestStart();
    if (result != oboe::Result::OK) {
        LOGE("Failed to start recording stream. Error: %s", oboe::convertToText(result));
        m_recordingStream->close();
        return false;
    }

    LOGI("Successfully started Oboe capture stream.");
    return true;
}

void OboeAudio::StopCapture() {
    if (m_recordingStream) {
        m_recordingStream->requestStop();
        m_recordingStream->close();
        m_recordingStream.reset();
        LOGI("Oboe capture stream stopped.");
    }
}

bool OboeAudio::PopAudioFrame(std::vector<int16_t>& outData, int exactSamples) {
    if (m_audioQueue.size() < exactSamples) {
        return false; // Not enough data for a full Opus frame yet
    }

    int16_t sample;
    for (int i = 0; i < exactSamples; ++i) {
        if (m_audioQueue.pop(sample)) {
            outData.push_back(sample);
        }
    }
    return true;
}

oboe::DataCallbackResult OboeAudio::onAudioReady(
    oboe::AudioStream *oboeStream, void *audioData, int32_t numFrames) 
{
    // CRITICAL: High Priority Audio Thread. ZERO allocations allowed here!
    auto* pcmData = static_cast<int16_t*>(audioData);
    int32_t channels = oboeStream->getChannelCount();

    for (int32_t i = 0; i < numFrames * channels; ++i) {
        if (!m_audioQueue.push(pcmData[i])) {
            // Buffer overflow - Network thread is too slow or we disconnected.
            // We intentionally drop the samples here to keep real-time latency tight.
            break;
        }
    }

    return oboe::DataCallbackResult::Continue;
}

} // namespace android
} // namespace audiostream
