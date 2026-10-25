import AVFoundation
import Accelerate

/// Adaptive real-time noise reducer using gain reduction + HPF + LPF + noise gate.
/// Creates a "personal bubble" by band-passing audio (200Hz-2kHz default).
/// Auto-adjusts gain and gate based on ambient noise level.
final class NoiseReducerProcessor {
    private var enabled = false
    private var adaptive = true
    private var baseGain: Float = 0.2
    private var gateThreshold: Float = 0.01
    private var hpfCutoff: Float = 200.0
    private var lpfCutoff: Float = 2000.0

    // HPF state (single-pole IIR)
    private var hpfPrevIn: Float = 0
    private var hpfPrevOut: Float = 0
    private var hpfAlpha: Float = 0

    // LPF state (single-pole IIR)
    private var lpfPrevOut: Float = 0
    private var lpfAlpha: Float = 0

    // Adaptive state
    private var noiseLevel: Float = 0
    private var currentGain: Float = 0.2
    private var currentGate: Float = 0.01

    init(sampleRate: Float = 48000) {
        updateHPFCoeff(sampleRate: sampleRate)
        updateLPFCoeff(sampleRate: sampleRate)
    }

    func setEnabled(_ e: Bool) { enabled = e }
    func isEnabled() -> Bool { enabled }

    func setAdaptive(_ a: Bool) { adaptive = a }
    func isAdaptive() -> Bool { adaptive }

    func setGain(_ g: Float) {
        baseGain = Swift.min(Swift.max(g, 0), 2)
        if !adaptive { currentGain = baseGain }
    }
    func getGain() -> Float { baseGain }
    func getCurrentGain() -> Float { currentGain }

    func setGateThreshold(_ t: Float) { gateThreshold = Swift.min(Swift.max(t, 0), 0.5) }
    func getGateThreshold() -> Float { return gateThreshold }

    func setHPFCutoff(_ freq: Float, sampleRate: Float = 48000) {
        hpfCutoff = Swift.min(Swift.max(freq, 20), 500)
        updateHPFCoeff(sampleRate: sampleRate)
    }

    func setLPFCutoff(_ freq: Float, sampleRate: Float = 48000) {
        lpfCutoff = Swift.min(Swift.max(freq, 500), 8000)
        updateLPFCoeff(sampleRate: sampleRate)
    }

    func getNoiseLevel() -> Float { noiseLevel }

    func reset() {
        hpfPrevIn = 0
        hpfPrevOut = 0
        lpfPrevOut = 0
        noiseLevel = 0
        currentGain = baseGain
        currentGate = gateThreshold
    }

    /// Process interleaved or non-interleaved float32 audio buffer with adaptive adjustment.
    func process(buffer: AVAudioPCMBuffer) {
        guard enabled,
              let channelData = buffer.floatChannelData else { return }

        let frameCount = Int(buffer.frameLength)
        let channelCount = Int(buffer.format.channelCount)

        // Measure RMS for adaptive adjustment
        if adaptive {
            var rms: Float = 0
            for ch in 0..<channelCount {
                let ptr = channelData[ch]
                for i in 0..<frameCount {
                    rms += ptr[i] * ptr[i]
                }
            }
            rms = sqrt(rms / Float(frameCount * channelCount))

            // Update noise level estimate (exponential moving average)
            noiseLevel = 0.95 * noiseLevel + 0.05 * rms

            // Adaptive gain: reduce gain when noise is loud, increase when quiet
            let targetRMS: Float = 0.05
            var adaptiveGain = targetRMS / Swift.max(noiseLevel, 0.001)
            adaptiveGain = Swift.min(Swift.max(adaptiveGain, 0.05), baseGain)
            currentGain = 0.9 * currentGain + 0.1 * adaptiveGain

            // Adaptive gate: raise threshold when noise floor is high
            let adaptiveGate = Swift.max(gateThreshold, noiseLevel * 0.5)
            currentGate = adaptiveGate
        } else {
            currentGain = baseGain
            currentGate = gateThreshold
        }

        let gain = currentGain
        let gate = currentGate

        for ch in 0..<channelCount {
            let ptr = channelData[ch]
            for i in 0..<frameCount {
                var sample = ptr[i]

                // Noise gate
                if abs(sample) < gate {
                    sample = 0
                }

                // High-pass filter (cuts engine rumble)
                let y = hpfAlpha * (hpfPrevOut + sample - hpfPrevIn)
                hpfPrevIn = sample
                hpfPrevOut = y
                sample = y

                // Low-pass filter (cuts tire hiss, brake noise)
                let z = lpfPrevOut + lpfAlpha * (sample - lpfPrevOut)
                lpfPrevOut = z
                sample = z

                // Gain reduction
                ptr[i] = sample * gain
            }
        }
    }

    private func updateHPFCoeff(sampleRate: Float) {
        let rc = 1.0 / (2.0 * Float.pi * hpfCutoff)
        let dt = 1.0 / sampleRate
        hpfAlpha = rc / (rc + dt)
    }

    private func updateLPFCoeff(sampleRate: Float) {
        let rc = 1.0 / (2.0 * Float.pi * lpfCutoff)
        let dt = 1.0 / sampleRate
        lpfAlpha = dt / (rc + dt)
    }
}
