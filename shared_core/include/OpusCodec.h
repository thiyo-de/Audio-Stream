#pragma once

#include <vector>
#include <cstdint>

struct OpusEncoder;
struct OpusDecoder;

namespace audiostream {
namespace codec {

class OpusAudioEncoder {
public:
    // Sample rate (e.g. 48000), channels (e.g. 1)
    OpusAudioEncoder(int sampleRate = 48000, int channels = 1);
    ~OpusAudioEncoder();

    // Encode raw PCM data. inputPcm must contain precisely one frame size of samples 
    // (e.g., 960 samples for 20ms at 48kHz).
    // Returns the encoded compressed payload.
    std::vector<uint8_t> Encode(const int16_t* inputPcm, int numSamples);

private:
    OpusEncoder* m_encoder = nullptr;
    int m_channels;
};

class OpusAudioDecoder {
public:
    OpusAudioDecoder(int sampleRate = 48000, int channels = 1);
    ~OpusAudioDecoder();

    // Decode compressed payload. Returns raw PCM samples.
    // numSamples is the expected frame size (e.g., 960).
    std::vector<int16_t> Decode(const uint8_t* compressedData, size_t length, int frameSize);

private:
    OpusDecoder* m_decoder = nullptr;
    int m_channels;
};

} // namespace codec
} // namespace audiostream
