package com.audiostream

class NativeEngine {
    companion object {
        init {
            System.loadLibrary("audiostream_jni")
        }
    }

    // Connects to the Windows host via the ADB forwarded port (localhost:5000)
    external fun connectToHost(ip: String, port: Int): Boolean
    
    // Send a text message to Windows
    external fun sendMessage(msg: String): Boolean
    
    // Stop transport
    external fun disconnect()
}
