#include <jni.h>
#include <string>
#include <vector>
#include <atomic>
#include <thread>
#include <android/log.h>
#include "TcpUsbTransport.h"
#include "OboeAudio.h"
#include "OboePlayback.h"
#include "OpusCodec.h"

#define LOG_TAG "NativeEngine"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

std::unique_ptr<audiostream::transport::TcpUsbTransport> g_transport;
std::unique_ptr<audiostream::android::OboeAudio> g_oboeAudio;
std::unique_ptr<audiostream::android::OboePlayback> g_oboePlayback;
std::unique_ptr<audiostream::codec::OpusAudioEncoder> g_opusEncoder;
std::unique_ptr<audiostream::codec::OpusAudioDecoder> g_opusDecoder;
std::atomic<bool> g_audioSenderRunning{false};
std::thread g_audioSenderThread;

extern "C" JNIEXPORT jboolean JNICALL
Java_com_audiostream_NativeEngine_connectToHost(JNIEnv* env, jobject /* this */, jstring ipStr, jint port) {
    const char* ipChars = env->GetStringUTFChars(ipStr, nullptr);
    std::string ip(ipChars);
    env->ReleaseStringUTFChars(ipStr, ipChars);

    LOGI("Attempting to connect to %s:%d", ip.c_str(), port);

    // false = Client mode
    g_transport = std::make_unique<audiostream::transport::TcpUsbTransport>(false, ip, port);
    
    g_transport->SetReceiveCallback([](const std::vector<uint8_t>& data) {
        if (g_opusDecoder && g_oboePlayback) {
            std::vector<int16_t> pcmData = g_opusDecoder->Decode(data.data(), data.size(), 960);
            if (!pcmData.empty()) {
                g_oboePlayback->PushAudioData(pcmData.data(), pcmData.size());
            }
        }
    });

    bool success = g_transport->Initialize();
    if (success) {
        LOGI("Successfully connected to host!");
        
        g_oboePlayback = std::make_unique<audiostream::android::OboePlayback>();
        g_oboeAudio = std::make_unique<audiostream::android::OboeAudio>();
        g_opusEncoder = std::make_unique<audiostream::codec::OpusAudioEncoder>(48000, 1);
        g_opusDecoder = std::make_unique<audiostream::codec::OpusAudioDecoder>(48000, 1);
        
        g_oboePlayback->StartPlayback();
        
        if (g_oboeAudio->StartCapture()) {
            g_audioSenderRunning = true;
            g_audioSenderThread = std::thread([]() {
                std::vector<int16_t> pcmBuffer;
                // 960 samples = 20ms at 48kHz
                const int FRAME_SIZE = 960;
                
                while (g_audioSenderRunning) {
                    if (g_oboeAudio->PopAudioFrame(pcmBuffer, FRAME_SIZE)) {
                        // Encode the raw PCM into tiny Opus compressed bytes
                        std::vector<uint8_t> opusPayload = g_opusEncoder->Encode(pcmBuffer.data(), FRAME_SIZE);
                        if (!opusPayload.empty()) {
                            g_transport->Send(opusPayload);
                        }
                        pcmBuffer.clear();
                    } else {
                        // Sleep briefly if not enough data yet
                        std::this_thread::sleep_for(std::chrono::milliseconds(2));
                    }
                }
            });
        }

    } else {
        LOGE("Failed to connect to host.");
    }
    
    return success;
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_audiostream_NativeEngine_sendMessage(JNIEnv* env, jobject /* this */, jstring msgStr) {
    if (!g_transport) return false;

    const char* msgChars = env->GetStringUTFChars(msgStr, nullptr);
    std::string msg(msgChars);
    env->ReleaseStringUTFChars(msgStr, msgChars);

    std::vector<uint8_t> payload(msg.begin(), msg.end());
    bool success = g_transport->Send(payload);
    
    return success;
}

extern "C" JNIEXPORT void JNICALL
Java_com_audiostream_NativeEngine_disconnect(JNIEnv* /* env */, jobject /* this */) {
    if (g_audioSenderRunning) {
        g_audioSenderRunning = false;
        if (g_audioSenderThread.joinable()) g_audioSenderThread.join();
    }
    
    if (g_oboeAudio) {
        g_oboeAudio->StopCapture();
        g_oboeAudio.reset();
    }
    
    if (g_oboePlayback) {
        g_oboePlayback->StopPlayback();
        g_oboePlayback.reset();
    }

    if (g_transport) {
        g_transport->Stop();
        g_transport.reset();
        LOGI("Disconnected.");
    }
}
