#include "OpusCodec.h"
#include <opus.h>
#include <iostream>

namespace audiostream {
namespace codec {

OpusAudioEncoder::OpusAudioEncoder(int sampleRate, int channels) : m_channels(channels) {
    int error = 0;
    // OPUS_APPLICATION_VOIP is optimized for speech (perfect for our use case)
    m_encoder = opus_encoder_create(sampleRate, channels, OPUS_APPLICATION_VOIP, &error);
    if (error != OPUS_OK) {
        std::cerr << "Failed to create Opus Encoder: " << opus_strerror(error) << std::endl;
    } else {
        // Set a low bitrate to keep network footprint tiny (e.g., 64000 = 64 kbps)
        opus_encoder_ctl(m_encoder, OPUS_SET_BITRATE(64000));
        // We want absolute lowest latency
        opus_encoder_ctl(m_encoder, OPUS_SET_COMPLEXITY(1));
        opus_encoder_ctl(m_encoder, OPUS_SET_SIGNAL(OPUS_SIGNAL_VOICE));
    }
}

OpusAudioEncoder::~OpusAudioEncoder() {
    if (m_encoder) {
        opus_encoder_destroy(m_encoder);
    }
}

std::vector<uint8_t> OpusAudioEncoder::Encode(const int16_t* inputPcm, int numSamples) {
    if (!m_encoder) return {};

    // Max Opus payload size is 4000 bytes
    std::vector<uint8_t> output(4000);
    int numBytes = opus_encode(m_encoder, inputPcm, numSamples, output.data(), static_cast<opus_int32>(output.size()));
    
    if (numBytes > 0) {
        output.resize(numBytes);
        return output;
    }
    
    // Error during encoding
    return {};
}

OpusAudioDecoder::OpusAudioDecoder(int sampleRate, int channels) : m_channels(channels) {
    int error = 0;
    m_decoder = opus_decoder_create(sampleRate, channels, &error);
    if (error != OPUS_OK) {
        std::cerr << "Failed to create Opus Decoder: " << opus_strerror(error) << std::endl;
    }
}

OpusAudioDecoder::~OpusAudioDecoder() {
    if (m_decoder) {
        opus_decoder_destroy(m_decoder);
    }
}

std::vector<int16_t> OpusAudioDecoder::Decode(const uint8_t* compressedData, size_t length, int frameSize) {
    if (!m_decoder) return {};

    std::vector<int16_t> pcmOutput(frameSize * m_channels);
    
    // Decode into our PCM buffer
    int decodedSamples = opus_decode(m_decoder, compressedData, static_cast<opus_int32>(length), pcmOutput.data(), frameSize, 0);
    
    if (decodedSamples > 0) {
        pcmOutput.resize(decodedSamples * m_channels);
        return pcmOutput;
    }

    return {};
}

} // namespace codec
} // namespace audiostream
