#pragma once

#include <oboe/Oboe.h>
#include <vector>
#include <memory>
#include "LockFreeQueue.h"

namespace audiostream {
namespace android {

class OboeAudio : public oboe::AudioStreamDataCallback {
public:
    OboeAudio();
    ~OboeAudio() override;

    bool StartCapture();
    void StopCapture();

    // The network thread calls this to grab the latest audio to send over the USB tunnel
    bool PopAudioData(std::vector<int16_t>& outData);

    // AudioStreamDataCallback override (called by OS Audio Thread)
    oboe::DataCallbackResult onAudioReady(
        oboe::AudioStream *oboeStream,
        void *audioData,
        int32_t numFrames) override;

private:
    std::shared_ptr<oboe::AudioStream> m_recordingStream;
    
    // We use int16_t for PCM audio.
    // 48000 Hz * 1 channel * 1.0 seconds = 48000 samples buffer.
    LockFreeQueue<int16_t> m_audioQueue; 
};

} // namespace android
} // namespace audiostream
