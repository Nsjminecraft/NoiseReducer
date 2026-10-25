import SwiftUI

struct ContentView: View {
    @StateObject private var audio = AudioEngine()

    var body: some View {
        NavigationView {
            Form {
                Section("Controls") {
                    Toggle("Enable Noise Reduction", isOn: $audio.isEnabled)
                    Toggle("Auto-Adjust (Adaptive)", isOn: $audio.isAdaptive)

                    Button(audio.isRunning ? "Stop" : "Start") {
                        if audio.isRunning {
                            audio.stop()
                        } else {
                            audio.start()
                        }
                    }
                    .disabled(!audio.isEnabled && !audio.isRunning)
                }

                Section("Status") {
                    HStack {
                        Text("Noise Level")
                        Spacer()
                        Text(String(format: "%.3f", audio.currentNoiseLevel))
                            .foregroundColor(.secondary)
                    }
                    HStack {
                        Text("Current Gain")
                        Spacer()
                        Text(String(format: "%.2f", audio.currentGainValue))
                            .foregroundColor(.secondary)
                    }
                }

                Section("Parameters") {
                    VStack(alignment: .leading) {
                        Text("Gain: \(audio.gain, specifier: "%.2f")")
                        Slider(value: $audio.gain, in: 0...1)
                    }

                    VStack(alignment: .leading) {
                        Text("Noise Gate: \(audio.gateThreshold, specifier: "%.3f")")
                        Slider(value: $audio.gateThreshold, in: 0...0.2)
                    }

                    VStack(alignment: .leading) {
                        Text("HPF Cutoff: \(audio.hpfCutoff, specifier: "%.0f") Hz")
                        Slider(value: $audio.hpfCutoff, in: 20...500)
                    }

                    VStack(alignment: .leading) {
                        Text("LPF Cutoff: \(audio.lpfCutoff, specifier: "%.0f") Hz")
                        Slider(value: $audio.lpfCutoff, in: 500...8000)
                    }

                    Button("Reset") {
                        audio.gain = 0.2
                        audio.gateThreshold = 0.01
                        audio.hpfCutoff = 200
                        audio.lpfCutoff = 2000
                        audio.isAdaptive = true
                        audio.resetProcessor()
                    }
                }

                if let error = audio.errorMessage {
                    Section("Error") {
                        Text(error).foregroundColor(.red)
                    }
                }

                Section {
                    Text("Captures mic → auto-adjusts → outputs to earbuds. Prefers Bluetooth earbuds mic. Adaptive mode auto-adjusts gain based on noise level.")
                        .font(.caption)
                        .foregroundColor(.secondary)
                }
            }
            .navigationTitle("Noise Reducer")
        }
    }
}
