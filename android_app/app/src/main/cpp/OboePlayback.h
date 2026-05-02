#pragma once

#include <oboe/Oboe.h>
#include <vector>
#include <memory>
#include "LockFreeQueue.h"

namespace audiostream {
namespace android {

class OboePlayback : public oboe::AudioStreamDataCallback {
public:
    OboePlayback();
    ~OboePlayback() override;

    bool StartPlayback();
    void StopPlayback();

    // The network thread pushes received decoded PCM here
    bool PushAudioData(const int16_t* data, size_t numFrames);

    // AudioStreamDataCallback override
    oboe::DataCallbackResult onAudioReady(
        oboe::AudioStream *oboeStream,
        void *audioData,
        int32_t numFrames) override;

private:
    std::shared_ptr<oboe::AudioStream> m_playbackStream;
    LockFreeQueue<int16_t> m_audioQueue; 
};

} // namespace android
} // namespace audiostream
