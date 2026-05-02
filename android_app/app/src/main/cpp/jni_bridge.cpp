#include <jni.h>
#include <string>
#include <vector>
#include <memory>
#include <android/log.h>
#include "TcpUsbTransport.h"

#define LOG_TAG "NativeEngine"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

std::unique_ptr<audiostream::transport::TcpUsbTransport> g_transport;

extern "C" JNIEXPORT jboolean JNICALL
Java_com_audiostream_NativeEngine_connectToHost(JNIEnv* env, jobject /* this */, jstring ipStr, jint port) {
    const char* ipChars = env->GetStringUTFChars(ipStr, nullptr);
    std::string ip(ipChars);
    env->ReleaseStringUTFChars(ipStr, ipChars);

    LOGI("Attempting to connect to %s:%d", ip.c_str(), port);

    // false = Client mode
    g_transport = std::make_unique<audiostream::transport::TcpUsbTransport>(false, ip, port);
    
    g_transport->SetReceiveCallback([](const std::vector<uint8_t>& data) {
        std::string text(data.begin(), data.end());
        LOGI("Received from Windows: %s", text.c_str());
    });

    bool success = g_transport->Initialize();
    if (success) {
        LOGI("Successfully connected to host!");
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
    if (g_transport) {
        g_transport->Stop();
        g_transport.reset();
        LOGI("Disconnected.");
    }
}
