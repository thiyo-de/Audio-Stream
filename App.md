# AudioStream: Ultimate Low-Latency Full-Duplex Audio Engine (God-Tier Architecture)

This document outlines a production-grade, highly resilient system architecture for a full-duplex, sub-50ms latency audio streaming platform (Android ↔ Windows). This is designed for an elite developer, addressing system-level bottlenecks that typically destroy casual audio implementations.

---

## 1. The "Godly" Tech Stack

To achieve absolute lowest latency and avoid memory leaks/garbage collection pauses, we must push the heavy lifting to native code on both ends.

### A. Windows Engine (The Host)

- **Core Language:** Modern C++20.
- **Audio API:** WASAPI (Windows Audio Session API) in **Exclusive Mode** (bypasses Windows audio mixer for lowest latency).
- **Concurrency:** `moodycamel::ReaderWriterQueue` (Single-Producer-Single-Consumer lock-free ring buffer).
- **Virtual Device:**
  - _Phase 1:_ VB-Cable / Virtual Audio Cable (via WASAPI loopback).
  - _Phase 2:_ Custom WDM/KS (Kernel Streaming) Virtual Audio Driver (written in C/C++).
- **Networking/Transport:** Asio (standalone) or pure WinSock with support for UDP, TCP, and ADB port forwarding.

### B. Android Engine (The Client)

- **UI & App Layer:** Kotlin + Jetpack Compose (MVVM).
- **Core Audio/Network Layer:** C++ via Android NDK (JNI bindings).
- **Audio API:** Oboe (Google's C++ library that wraps AAudio and OpenSL ES).
- **Networking/Transport:** POSIX sockets, Android USB Accessory API (AOA), and WiFi Direct.

### C. Shared Cross-Platform Libraries (C++)

- **Codec:** `libopus` (Opus Interactive Audio Codec - configured for low delay/VoIP).
- **DSP / Resampling:** `SpeexDSP` or `libsamplerate` (crucial for clock drift).
- **Acoustic Echo Cancellation (AEC):** WebRTC DSP library.

---

## 2. Anticipated System-Level Issues & The God-Tier Solutions

Building an audio engine is a fight against time and network infrastructure. Here is every issue that _will_ happen and how we mitigate it.

### 🔴 1. Corporate Network Isolation (Ethernet vs. WiFi)

**The Problem:** In office networks (like Jio), the PC is often on Ethernet and the Phone on WiFi. Corporate routers enforce VLAN separation and "Client Isolation," meaning the phone cannot ping the PC, rendering standard UDP streaming useless.
**The God-Tier Solution:**

- **Transport Agnostic Design:** The audio engine must not care _how_ packets are sent. We build multiple modular transports:
  - **Primary (USB-First):** Use `adb forward tcp:X tcp:X` or Android Open Accessory (AOA) protocol. This routes traffic via the USB cable directly, guaranteeing 0% packet loss, bypasses office firewalls, and works instantly.
  - **Secondary (WiFi Direct / Hotspot):** The Android app programmatically creates a local hotspot or uses WiFi Direct, allowing the PC to connect to the phone directly, completely ignoring the office router.
  - **Tertiary (LAN UDP):** Only if the devices can successfully ping each other on the same subnet.

### 🔴 2. Clock Drift (The Silent Killer)

**The Problem:** The hardware oscillator on the Android mic runs at 48000.01Hz, and the Windows playback runs at 47999.98Hz. Over a few minutes, the buffer will either completely drain (underrun/clicking) or overflow (lag).
**The God-Tier Solution:**

- Implement a **Dynamic Resampler**. Monitor the size of the receiver's jitter buffer. If the buffer is growing, dynamically increase the playback sample rate slightly (e.g., to 48005Hz) using `SpeexDSP`. If it's shrinking, slow it down.

### 🔴 3. Audio Thread Preemption (Glitches & Dropouts)

**The Problem:** The OS scheduler pauses the audio thread for 5ms to do background work, missing the 2ms audio deadline. Audio cracks.
**The God-Tier Solution:**

- **Windows:** Register the audio thread with **MMCSS** (Multimedia Class Scheduler Service) using `AvSetMmThreadCharacteristics("Pro Audio", ...)`.
- **Android:** Request `PERFORMANCE_MODE_LOW_LATENCY` in Oboe and ensure the audio callback thread is never blocked.

### 🔴 4. Memory Allocation in the Real-Time Path (Priority Inversion)

**The Problem:** Using `new`, `malloc`, or Kotlin object creation inside the audio callback triggers an OS lock or Garbage Collection, causing a deadline miss.
**The God-Tier Solution:**

- The audio callback must be 100% allocation-free and lock-free. Use lock-free SPSC (Single Producer, Single Consumer) ring buffers to pass PCM data between the network thread and the audio thread.

### 🔴 5. Network Jitter & Out-of-Order Packets

**The Problem:** WiFi is inherently unstable. Packets arrive late, out of order, or clumped together.
**The God-Tier Solution:**

- Implement a **Dynamic Jitter Buffer** (like WebRTC's NetEQ). Attach Sequence Numbers and Timestamps to every packet. Conceal lost packets using Opus's built-in **FEC (Forward Error Correction)** and **PLC (Packet Loss Concealment)**.

### 🔴 6. Acoustic Echo (Feedback Loops)

**The Problem:** When streaming Windows audio to the phone, the phone's speaker plays it, and the phone's mic immediately captures it and sends it back to Windows, creating a deafening screech.
**The God-Tier Solution:**

- Integrate the **WebRTC AEC3** (Acoustic Echo Cancellation) module in the Android NDK.

---

## 3. The God-Tier File Architecture

To support our Transport Agnostic design, the network logic is heavily decoupled from the audio logic.

### Common Code (C++ CMake) -> Shared via Git Submodule

```text
shared_core/
├── include/
│   ├── LockFreeQueue.h      # SPSC ring buffer implementation
│   ├── Protocol.h           # Packet headers (Seq, Timestamp, FEC data)
│   ├── OpusWrapper.h        # C++ wrapper for libopus encoder/decoder
│   └── Resampler.h          # Dynamic resampling logic for clock drift
├── transport/               # Transport-Agnostic Layer
│   ├── ITransport.h         # Interface: Send(), Receive(), OnConnect()
│   ├── UdpTransport.h       # For open LAN environments
│   └── TcpUsbTransport.h    # For ADB Port Forwarding / USB
├── src/
│   ├── OpusWrapper.cpp
│   └── Resampler.cpp
└── third_party/             # libopus, moodycamel, SpeexDSP
```

### Android Repository (Kotlin + NDK)

```text
android_app/
├── app/src/main/
│   ├── java/com/audiostream/
│   │   ├── MainActivity.kt        # Jetpack Compose UI
│   │   ├── EngineManager.kt       # JNI bindings definition
│   │   ├── NetworkService.kt      # Foreground service to keep app alive
│   │   └── UsbConnectionMngr.kt   # Detects USB plugging / ADB status
│   └── cpp/
│       ├── CMakeLists.txt
│       ├── JniBridge.cpp          # Bridges Kotlin -> NativeEngine
│       ├── NativeEngine.cpp       # Orchestrates audio + transport
│       ├── OboeCapture.cpp        # Low-latency Mic input
│       ├── OboePlayback.cpp       # Low-latency Speaker output
│       ├── SocketTransports.cpp   # Implements ITransport for Android
│       └── WebRtcAec.cpp          # Echo cancellation integration
```

### Windows Repository (Modern C++ / MSVC)

```text
windows_app/
├── src/
│   ├── main.cpp                   # App entry / Win32 or Qt UI
│   ├── AudioEngine.cpp            # Orchestrator
│   ├── WasapiLoopback.cpp         # Captures system audio
│   ├── WasapiRender.cpp           # Feeds audio to Virtual Mic
│   ├── TransportManager.cpp       # Handles USB ADB / UDP switching
│   └── MMCSSManager.cpp           # Elevates threads to Pro Audio
├── scripts/
│   └── setup_adb_tunnel.bat       # Auto-runs "adb forward tcp:5000 tcp:5000"
├── virtual_driver/                # (Phase 2) WDM Kernel Driver
│   ├── AudioStream.inf
│   └── miniport.cpp
```

---

## 4. Execution Roadmap (Phased Approach)

### Phase 1: USB Tunneling & Raw Architecture (The Firewall Bypass)

1. Setup C++ CMake projects. Implement `ITransport` with `TcpUsbTransport`.
2. Write a script on the Windows side that automatically runs `adb forward tcp:5000 tcp:5000` when the phone is plugged in.
3. Test a raw socket connection over USB. This bypasses the office ethernet/WiFi problem completely.

### Phase 2: Audio I/O & NDK

1. **Android:** Implement Oboe audio capture and playback. Send/receive dummy sine waves.
2. **Windows:** Implement WASAPI Loopback and Render. Send/receive dummy sine waves.
3. **Integration:** Connect Phase 1 transport to Phase 2 audio queues.

### Phase 3: Codec & Resilience (The "Godly" Layer)

1. Integrate `libopus` (20ms frames, 16-48kbps).
2. Implement Sequence numbers, Timestamps, and the Dynamic Jitter Buffer.
3. Implement Opus PLC and Dynamic Resampling for Clock Drift.

### Phase 4: Office Networking Fallbacks

1. Implement the `UdpTransport` for open LANs.
2. Implement Android code to programmatically spin up a WiFi Direct connection or Hotspot if USB is disconnected and LAN UDP fails.

### Phase 5: The Virtual Kernel Driver

1. Replace VB-Cable dependency.
2. Write a custom Windows WDM/KS virtual audio driver to expose "AudioStream Mic" natively to the OS.

git init
git add README.md
git commit -m "first commit"
git branch -M main
git remote add origin https://github.com/thiyo-de/Audio-Stream.git
git push -u origin main
