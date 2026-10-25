import AVFoundation
import Combine

/// Manages AVAudioEngine for real-time mic capture → adaptive noise reduction → output.
/// Prefers Bluetooth earbuds mic when available.
final class AudioEngine: ObservableObject {
    private let engine = AVAudioEngine()
    private let processor = NoiseReducerProcessor()

    @Published var isEnabled = false {
        didSet { processor.setEnabled(isEnabled) }
    }
    @Published var isAdaptive = true {
        didSet { processor.setAdaptive(isAdaptive) }
    }
    @Published var gain: Float = 0.2 {
        didSet { processor.setGain(gain) }
    }
    @Published var gateThreshold: Float = 0.01 {
        didSet { processor.setGateThreshold(gateThreshold) }
    }
    @Published var hpfCutoff: Float = 200 {
        didSet { processor.setHPFCutoff(hpfCutoff) }
    }
    @Published var lpfCutoff: Float = 2000 {
        didSet { processor.setLPFCutoff(lpfCutoff) }
    }
    @Published var isRunning = false
    @Published var errorMessage: String?
    @Published var currentNoiseLevel: Float = 0
    @Published var currentGainValue: Float = 0.2

    private var levelTimer: Timer?

    func start() {
        guard !isRunning else { return }

        let session = AVAudioSession.sharedInstance()
        do {
            // Configure for lowest latency with Bluetooth support
            try session.setCategory(.playAndRecord, mode: .voiceChat,
                                   options: [.defaultToSpeaker, .allowBluetooth, .allowBluetoothA2DP])
            try session.setPreferredIOBufferDuration(0.005) // 5ms target
            try session.setPreferredSampleRate(48000)
            try session.setActive(true)

            // Try to route input to Bluetooth device if available
            selectBluetoothInput()
        } catch {
            errorMessage = "Audio session error: \(error.localizedDescription)"
            return
        }

        let inputNode = engine.inputNode
        let format = inputNode.outputFormat(forBus: 0)

        // Install tap on input for real-time processing
        inputNode.installTap(onBus: 0, bufferSize: 256, format: format) { [weak self] buffer, time in
            self?.processor.process(buffer: buffer)
        }

        // Route input → output
        let outputNode = engine.outputNode
        engine.connect(inputNode, to: outputNode, format: format)

        do {
            try engine.start()
            isRunning = true
            processor.setEnabled(isEnabled)

            // Start timer to update noise level display
            levelTimer = Timer.scheduledTimer(withTimeInterval: 0.1, repeats: true) { [weak self] _ in
                if let self = self {
                    self.currentNoiseLevel = self.processor.getNoiseLevel()
                    self.currentGainValue = self.processor.getCurrentGain()
                }
            }
        } catch {
            errorMessage = "Engine start error: \(error.localizedDescription)"
        }
    }

    func stop() {
        guard isRunning else { return }
        levelTimer?.invalidate()
        levelTimer = nil
        engine.inputNode.removeTap(onBus: 0)
        engine.stop()
        isRunning = false
    }

    func resetProcessor() {
        processor.reset()
    }

    private func selectBluetoothInput() {
        let session = AVAudioSession.sharedInstance()
        let availableInputs = session.availableInputs ?? []

        // Look for Bluetooth device
        for port in availableInputs {
            let portName = port.portName.lowercased()
            if portName.contains("bluetooth") || portName.contains("airpods") ||
               portName.contains("beats") || port.portType == .bluetoothHFP {
                do {
                    try session.setPreferredInput(port)
                    print("Selected Bluetooth input: \(port.portName)")
                    return
                } catch {
                    print("Failed to select Bluetooth input: \(error)")
                }
            }
        }

        print("No Bluetooth input found, using default")
    }
}
