# AudioStream: Execution Tasks

This document tracks the live progress of the AudioStream project.

## ✅ Phase 1: Foundation & USB Tunnel (COMPLETED)
- [x] **Monorepo Setup:** Scaffolding for `shared_core`, `android_app`, and `windows_app`.
- [x] **Network Transport Abstraction:** Implement `ITransport.h`.
- [x] **ADB USB Firewall Bypass:** Create `setup_adb_tunnel.bat` script.
- [x] **Zero-Latency TCP Implementation:** Create `TcpUsbTransport` C++ class.
- [x] **Windows App Scaffold:** Server waiting for connection.
- [x] **Android App Scaffold:** Kotlin UI & JNI bindings connecting to server.

## ⏳ Phase 2: Raw Audio Processing
*We have the network pipe; now we need to hook up microphones and speakers.*
- [ ] **Android Oboe Integration:** Integrate Google's Oboe C++ Library to capture raw microphone data at ultra-low latency.
- [ ] **Windows WASAPI Integration:** Use WASAPI in Exclusive Mode to capture PC system audio and play incoming Android audio.
- [ ] **Lock-Free Queues:** Implement `moodycamel::ReaderWriterQueue` to pass audio between network and speaker threads without garbage collection spikes.

## ⏳ Phase 3: Codec & Resilience (The "Godly" Layer)
*Raw audio is too large. We need compression and stability.*
- [ ] **Opus Codec Integration:** Wrap `libopus` to compress 20ms audio chunks from ~3.8mbps down to ~32kbps without quality loss.
- [ ] **Dynamic Jitter Buffer:** Algorithm to briefly hold audio packets to smooth out network stutters.
- [ ] **Dynamic Resampler (SpeexDSP):** Fix Clock Drift by dynamically syncing Android and Windows clocks over long sessions.
- [ ] **Acoustic Echo Cancellation (AEC):** Prevent feedback loops using WebRTC DSP.

## ⏳ Phase 4: Office Networking Fallbacks
*Alternatives to using the USB cable every time.*
- [ ] **WiFi Direct / Local Hotspot:** Allow the Android app to automatically spin up a private WiFi network, bypassing strict office Ethernet isolation.
- [ ] **UDP Transport:** Build `UdpTransport.cpp` as a fallback for standard home WiFi networks.

## ⏳ Phase 5: Virtual Microphone Integration
*Routing audio as a real system microphone.*
- [ ] **VB-Cable Hook:** Route the WASAPI audio output into Virtual Audio Cable as an interim solution.
- [ ] **Windows WDM/KS Virtual Driver:** Write a custom Windows C++ driver to expose the system natively as an "AudioStream Mic" device.
