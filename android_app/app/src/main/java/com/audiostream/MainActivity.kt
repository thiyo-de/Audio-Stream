package com.audiostream

import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.compose.foundation.layout.*
import androidx.compose.material3.Button
import androidx.compose.material3.Text
import androidx.compose.material3.TextField
import androidx.compose.runtime.*
import androidx.compose.ui.Modifier
import androidx.compose.ui.unit.dp

class MainActivity : ComponentActivity() {
    private val nativeEngine = NativeEngine()

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContent {
            var connectionStatus by remember { mutableStateOf("Disconnected") }
            var message by remember { mutableStateOf("") }

            Column(modifier = Modifier.padding(16.dp)) {
                Text("AudioStream Client (Android)")
                Spacer(modifier = Modifier.height(16.dp))
                
                Text("Status: $connectionStatus")
                Spacer(modifier = Modifier.height(16.dp))

                Button(onClick = {
                    // 127.0.0.1 is used because ADB tunnel forwards PC 5000 -> Android 5000
                    val success = nativeEngine.connectToHost("127.0.0.1", 5000)
                    connectionStatus = if (success) "Connected via ADB Tunnel!" else "Connection Failed"
                }) {
                    Text("Connect via ADB USB Tunnel")
                }

                Spacer(modifier = Modifier.height(16.dp))

                TextField(
                    value = message,
                    onValueChange = { message = it },
                    label = { Text("Test Message") }
                )
                
                Button(onClick = {
                    nativeEngine.sendMessage(message)
                }, enabled = connectionStatus.contains("Connected")) {
                    Text("Send to Windows")
                }
            }
        }
    }

    override fun onDestroy() {
        super.onDestroy()
        nativeEngine.disconnect()
    }
}
