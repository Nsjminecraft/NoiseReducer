# NoiseReducer

Cross-platform passive noise reduction for any earbuds/earphones using your phone or computer's microphone. Reduces ambient background noise by applying gain reduction, high-pass filtering, and noise gating to the microphone input, then plays the processed audio back through your connected Bluetooth or wired headphones.

**Platforms:** Desktop (Linux/Windows/macOS), iOS, Android

## How It Works

The app captures audio from your device's microphone, processes it in real-time, and outputs the reduced ambient sound through your headphones. This creates a passive noise cancellation effect — background noise becomes quieter, not louder.

**Bluetooth earbuds mic:** The app automatically detects and uses your Bluetooth earbuds' microphone instead of the phone/device mic, so the audio processing happens close to the source.

**Adaptive mode:** By default, the app auto-adjusts gain and noise gate based on ambient noise level. When it's loud, gain drops to keep output quiet. When it's quiet, gain increases slightly. This creates a consistent "personal bubble" regardless of environment.

**Processing pipeline:**
1. **Noise gate** — mutes audio below a threshold (eliminates quiet hiss)
2. **High-pass filter** — removes low-frequency rumble (200Hz default, cuts engine noise)
3. **Low-pass filter** — removes high-frequency hiss (2kHz default, cuts tire/brake noise)
4. **Adaptive gain reduction** — auto-adjusts volume (base 0.2x, adapts to noise level)

This creates a "personal bubble" effect — car engines, tire hiss, and brake noise become much less noticeable while voices and important sounds remain audible.

**Why passive, not active?**
True active noise cancellation requires sub-millisecond latency to create an anti-phase signal. Bluetooth has 100-300ms latency, making true ANC impossible wirelessly. Passive noise reduction works within Bluetooth's constraints by simply making ambient sound quieter.

## Desktop (CLI)

**Requirements:** PortAudio, CMake

**Build (Linux/macOS):**
```bash
cd desktop
mkdir build && cd build
cmake ..
make
```

**Build (Windows):**
```bash
cd desktop
mkdir build && cd build
cmake -G "MinGW Makefiles" ..
mingw32-make
```

**Usage:**
```bash
./noise_reducer [options]

Options:
  --gain <0.0-2.0>     Base output gain (default: 0.2, overridden by adaptive mode)
  --gate <0.0-0.5>     Noise gate threshold (default: 0.01)
  --hpf <20-500>       High-pass filter cutoff Hz (default: 200)
  --lpf <500-8000>     Low-pass filter cutoff Hz (default: 2000)
  --no-adaptive        Disable auto-adjusting mode
  --device <name>      Input device name (e.g. 'Bluetooth', 'AirPods')
  --list-devices       List available audio devices
  --disable            Start disabled
```

**Examples:**
```bash
# Auto-adjusting mode with Bluetooth earbuds (default)
./noise_reducer

# List devices to find Bluetooth input name
./noise_reducer --list-devices

# Use specific Bluetooth device
./noise_reducer --device "AirPods"

# Manual mode (no auto-adjust)
./noise_reducer --no-adaptive --gain 0.15 --hpf 250 --lpf 3000
```

Connect your Bluetooth earbuds, run the app, and ambient noise will be quieter.

## iOS App

**Requirements:** Xcode 15+, iOS 17+

**Build:**
```bash
cd ios/NoiseReducer
# Open NoiseReducer.xcodeproj in Xcode, or:
xcodebuild -scheme NoiseReducer -destination 'platform=iOS Simulator,name=iPhone 15'
```

Or generate the project from `project.yml` using XcodeGen:
```bash
cd ios/NoiseReducer
xcodegen generate
open NoiseReducer.xcodeproj
```

**Usage:**
1. Connect Bluetooth earbuds to your iPhone/iPad
2. Launch the app
3. Grant microphone permission
4. Toggle "Auto-Adjust (Adaptive)" ON (default) for automatic gain adjustment
5. Tap "Start" to begin noise reduction
6. The app will automatically use your Bluetooth earbuds' microphone
7. Adjust sliders for base gain, gate threshold, high-pass filter, and low-pass filter
8. Monitor real-time noise level and current gain in the Status section

## Android App

**Requirements:** Android Studio, SDK 34, NDK (for C++ build)

**Build:**
```bash
cd /path/to/ANC
./gradlew assembleDebug
```

Or open the project in Android Studio and build from there.

**Usage:**
1. Connect Bluetooth earbuds to your Android device
2. Launch the app
3. Grant microphone permission
4. Toggle "Auto-Adjust (Adaptive)" ON (default) for automatic gain adjustment
5. Tap "Enable" to start noise reduction
6. The app will automatically detect and use your Bluetooth earbuds' microphone
7. Adjust sliders for base gain, gate, HPF, and LPF
8. Monitor real-time noise level and current gain display

## Architecture

**Desktop:** C++ with PortAudio for cross-platform audio I/O. Auto-detects Bluetooth input devices.
**iOS:** Swift with AVAudioEngine. Routes audio through Bluetooth HFP mic when available.
**Android:** Kotlin + C++ (JNI) with AudioRecord/AudioTrack. Routes audio through Bluetooth SCO mic.

All three platforms use the same core algorithm: adaptive gain control + single-pole IIR high-pass filter + single-pole IIR low-pass filter + noise gate. The band-pass filter (200Hz-2kHz) creates the "personal bubble" by cutting engine rumble and tire hiss. Adaptive mode measures RMS level and auto-adjusts gain to maintain consistent output regardless of ambient noise level.

## Tuning

- **Adaptive mode (default):** Auto-adjusts gain based on noise level. Keep this ON for consistent personal bubble in varying environments.
- **Manual mode:** Disable adaptive for fixed gain. Useful if you want consistent behavior regardless of environment.
- **More noise reduction:** Lower the base gain (0.1 = very quiet, 1.0 = passthrough)
- **Remove engine rumble:** Increase HPF cutoff (200-300Hz)
- **Remove tire hiss/brake noise:** Decrease LPF cutoff (1000-2000Hz)
- **Wider passband:** Increase LPF or decrease HPF to let more frequencies through
- **Mute quiet hiss:** Increase gate threshold (0.02-0.05)
- **Let more sound through:** Increase base gain above 0.2, or decrease gate

**Personal bubble defaults:** HPF 200Hz + LPF 2000Hz + gain 0.2 creates a focused bubble that cuts most traffic noise while keeping voices audible.

## License

MIT
