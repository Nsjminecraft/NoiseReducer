#include "noise_reducer.h"
#include <portaudio.h>
#include <iostream>
#include <atomic>
#include <cstring>

static std::atomic<bool> running{true};
static NoiseReducer* g_reducer = nullptr;

// PortAudio callback
static int audioCallback(const void* input, void* output,
                        unsigned long frameCount,
                        const PaStreamCallbackTimeInfo* timeInfo,
                        PaStreamCallbackFlags statusFlags,
                        void* userData) {
    float* out = (float*)output;
    const float* in = (const float*)input;

    if (in == nullptr) {
        std::memset(out, 0, frameCount * sizeof(float));
        return paContinue;
    }

    // Copy input to output buffer
    std::memcpy(out, in, frameCount * sizeof(float));

    // Process
    if (g_reducer) {
        g_reducer->process(out, frameCount);
    }

    return paContinue;
}

void printUsage(const char* prog) {
    std::cout << "Usage: " << prog << " [options]\n"
              << "Options:\n"
              << "  --gain <0.0-2.0>     Output gain (default: 0.2)\n"
              << "  --gate <0.0-0.5>     Noise gate threshold (default: 0.01)\n"
              << "  --hpf <20-500>       High-pass filter cutoff Hz (default: 200)\n"
              << "  --lpf <500-8000>     Low-pass filter cutoff Hz (default: 2000)\n"
              << "  --no-adaptive        Disable auto-adjusting mode\n"
              << "  --device <name>      Input device name substring (e.g. 'Bluetooth')\n"
              << "  --disable            Start with processing disabled\n"
              << "  --list-devices       List available audio devices\n"
              << "  --help               Show this help\n";
}

int findBluetoothDevice() {
    int numDevices = Pa_GetDeviceCount();
    const PaDeviceInfo* deviceInfo;
    std::string btKeywords[] = {"bluetooth", "bt", "airpods", "beats", "sony", "bose", "jbl"};

    for (int i = 0; i < numDevices; i++) {
        deviceInfo = Pa_GetDeviceInfo(i);
        if (deviceInfo->maxInputChannels > 0) { // input device
            std::string name = deviceInfo->name;
            std::string nameLower = name;
            std::transform(nameLower.begin(), nameLower.end(), nameLower.begin(), ::tolower);

            for (const auto& keyword : btKeywords) {
                if (nameLower.find(keyword) != std::string::npos) {
                    std::cout << "Found Bluetooth input device: " << name << "\n";
                    return i;
                }
            }
        }
    }
    return paNoDevice;
}

int findDeviceByName(const std::string& search) {
    int numDevices = Pa_GetDeviceCount();
    std::string searchLower = search;
    std::transform(searchLower.begin(), searchLower.end(), searchLower.begin(), ::tolower);

    for (int i = 0; i < numDevices; i++) {
        const PaDeviceInfo* deviceInfo = Pa_GetDeviceInfo(i);
        if (deviceInfo->maxInputChannels > 0) {
            std::string name = deviceInfo->name;
            std::string nameLower = name;
            std::transform(nameLower.begin(), nameLower.end(), nameLower.begin(), ::tolower);

            if (nameLower.find(searchLower) != std::string::npos) {
                return i;
            }
        }
    }
    return paNoDevice;
}

void listDevices() {
    int numDevices = Pa_GetDeviceCount();
    std::cout << "Available input devices:\n";
    for (int i = 0; i < numDevices; i++) {
        const PaDeviceInfo* deviceInfo = Pa_GetDeviceInfo(i);
        if (deviceInfo->maxInputChannels > 0) {
            std::cout << "  [" << i << "] " << deviceInfo->name
                     << " (channels: " << deviceInfo->maxInputChannels << ")\n";
        }
    }
}

int main(int argc, char* argv[]) {
    float gain = 0.2f;
    float gate = 0.01f;
    float hpf = 200.0f;
    float lpf = 2000.0f;
    bool startEnabled = true;
    bool adaptive = true;
    std::string deviceName;

    // Parse arguments
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--gain" && i + 1 < argc) {
            gain = std::stof(argv[++i]);
        } else if (arg == "--gate" && i + 1 < argc) {
            gate = std::stof(argv[++i]);
        } else if (arg == "--hpf" && i + 1 < argc) {
            hpf = std::stof(argv[++i]);
        } else if (arg == "--lpf" && i + 1 < argc) {
            lpf = std::stof(argv[++i]);
        } else if (arg == "--no-adaptive") {
            adaptive = false;
        } else if (arg == "--device" && i + 1 < argc) {
            deviceName = argv[++i];
        } else if (arg == "--disable") {
            startEnabled = false;
        } else if (arg == "--list-devices") {
            PaError err = Pa_Initialize();
            if (err != paNoError) {
                std::cerr << "PortAudio error: " << Pa_GetErrorText(err) << "\n";
                return 1;
            }
            listDevices();
            Pa_Terminate();
            return 0;
        } else if (arg == "--help") {
            printUsage(argv[0]);
            return 0;
        } else {
            std::cerr << "Unknown option: " << arg << "\n";
            printUsage(argv[0]);
            return 1;
        }
    }

    // Initialize PortAudio
    PaError err = Pa_Initialize();
    if (err != paNoError) {
        std::cerr << "PortAudio error: " << Pa_GetErrorText(err) << "\n";
        return 1;
    }

    // Select input device
    int inputDevice = paNoDevice;
    if (!deviceName.empty()) {
        inputDevice = findDeviceByName(deviceName);
        if (inputDevice == paNoDevice) {
            std::cerr << "Device not found: " << deviceName << "\n";
            listDevices();
            Pa_Terminate();
            return 1;
        }
        std::cout << "Using specified device: " << Pa_GetDeviceInfo(inputDevice)->name << "\n";
    } else {
        // Try to find Bluetooth device first
        inputDevice = findBluetoothDevice();
        if (inputDevice == paNoDevice) {
            std::cout << "No Bluetooth device found, using default input\n";
            inputDevice = Pa_GetDefaultInputDevice();
        }
    }

    if (inputDevice == paNoDevice) {
        std::cerr << "No input device found\n";
        Pa_Terminate();
        return 1;
    }

    // Get sample rate (use 48kHz)
    double sampleRate = 48000.0;

    // Create noise reducer
    NoiseReducer reducer((int)sampleRate);
    reducer.set_gain(gain);
    reducer.set_gate_threshold(gate);
    reducer.set_hpf_cutoff(hpf);
    reducer.set_lpf_cutoff(lpf);
    reducer.set_adaptive(adaptive);
    reducer.set_enabled(startEnabled);
    g_reducer = &reducer;

    // Open audio stream
    PaStreamParameters inputParams, outputParams;
    inputParams.device = inputDevice;
    inputParams.channelCount = 1; // mono
    inputParams.sampleFormat = paFloat32;
    inputParams.suggestedLatency = Pa_GetDeviceInfo(inputParams.device)->defaultLowInputLatency;
    inputParams.hostApiSpecificStreamInfo = nullptr;

    outputParams.device = Pa_GetDefaultOutputDevice();
    if (outputParams.device == paNoDevice) {
        std::cerr << "No output device found\n";
        Pa_Terminate();
        return 1;
    }
    outputParams.channelCount = 1; // mono
    outputParams.sampleFormat = paFloat32;
    outputParams.suggestedLatency = Pa_GetDeviceInfo(outputParams.device)->defaultLowOutputLatency;
    outputParams.hostApiSpecificStreamInfo = nullptr;

    PaStream* stream;
    err = Pa_OpenStream(&stream, &inputParams, &outputParams,
                       sampleRate, 256, // frames per buffer
                       paClipOff, audioCallback, nullptr);
    if (err != paNoError) {
        std::cerr << "PortAudio error: " << Pa_GetErrorText(err) << "\n";
        Pa_Terminate();
        return 1;
    }

    err = Pa_StartStream(stream);
    if (err != paNoError) {
        std::cerr << "PortAudio error: " << Pa_GetErrorText(err) << "\n";
        Pa_CloseStream(stream);
        Pa_Terminate();
        return 1;
    }

    std::cout << "Noise reduction active (personal bubble mode)\n"
              << "  Input: " << Pa_GetDeviceInfo(inputParams.device)->name << "\n"
              << "  Adaptive: " << (adaptive ? "ON" : "OFF") << "\n"
              << "  Gain: " << gain << "\n"
              << "  Gate: " << gate << "\n"
              << "  HPF: " << hpf << " Hz (cuts engine rumble)\n"
              << "  LPF: " << lpf << " Hz (cuts tire hiss)\n"
              << "  Enabled: " << (startEnabled ? "yes" : "no") << "\n"
              << "Press Ctrl+C to stop\n";

    // Wait for interrupt
    while (running.load()) {
        Pa_Sleep(100);
    }

    // Cleanup
    Pa_StopStream(stream);
    Pa_CloseStream(stream);
    Pa_Terminate();

    std::cout << "Stopped\n";
    return 0;
}
