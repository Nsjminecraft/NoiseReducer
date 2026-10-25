#pragma once

#include <vector>
#include <cmath>
#include <algorithm>
#include <atomic>
#include <cstring>

// Adaptive passive noise reduction: auto-adjusts gain and filters based on ambient noise level.
// Creates a "personal bubble" by band-passing audio (200Hz-2kHz default).
// ponytail: simple single-pole HPF + adaptive gain. Upgrade path: multi-band EQ, FFT-based notch filters.

class NoiseReducer {
public:
    NoiseReducer(int sample_rate = 48000)
        : sample_rate_(sample_rate)
        , hpf_cutoff_(200.0f)
        , lpf_cutoff_(2000.0f)
        , base_gain_(0.2f)
    {
        update_hpf_coeff();
        update_lpf_coeff();
        reset_state();
    }

    // Process audio in-place (mono, float32) with adaptive adjustment.
    void process(float* buffer, int num_frames) {
        bool enabled = enabled_.load(std::memory_order_relaxed);
        if (!enabled) return;

        bool adaptive = adaptive_.load(std::memory_order_relaxed);
        float gate = gate_threshold_.load(std::memory_order_relaxed);

        // Measure RMS for adaptive adjustment
        float rms = 0.0f;
        if (adaptive) {
            for (int i = 0; i < num_frames; i++) {
                rms += buffer[i] * buffer[i];
            }
            rms = std::sqrt(rms / num_frames);

            // Update noise level estimate (exponential moving average)
            noise_level_ = 0.95f * noise_level_ + 0.05f * rms;

            // Adaptive gain: reduce gain when noise is loud, increase when quiet
            float target_rms = 0.05f;
            float adaptive_gain = target_rms / std::max(noise_level_, 0.001f);
            adaptive_gain = std::clamp(adaptive_gain, 0.05f, base_gain_.load());
            current_gain_ = 0.9f * current_gain_ + 0.1f * adaptive_gain;

            // Adaptive gate: raise threshold when noise floor is high
            float adaptive_gate = std::max(gate, noise_level_ * 0.5f);
            current_gate_ = adaptive_gate;
        } else {
            current_gain_ = base_gain_.load();
            current_gate_ = gate;
        }

        float gain = current_gain_;
        float gate_now = current_gate_;

        for (int i = 0; i < num_frames; i++) {
            float x = buffer[i];

            // Noise gate
            if (std::abs(x) < gate_now) {
                x = 0.0f;
            }

            // High-pass filter
            float y = hpf_alpha_ * (hpf_prev_out_ + x - hpf_prev_in_);
            hpf_prev_in_ = x;
            hpf_prev_out_ = y;
            x = y;

            // Low-pass filter
            float z = lpf_prev_out_ + lpf_alpha_ * (x - lpf_prev_out_);
            lpf_prev_out_ = z;
            x = z;

            // Apply gain
            buffer[i] = x * gain;
        }
    }

    void set_enabled(bool e) { enabled_.store(e, std::memory_order_relaxed); }
    bool is_enabled() const { return enabled_.load(std::memory_order_relaxed); }

    void set_adaptive(bool a) { adaptive_.store(a, std::memory_order_relaxed); }
    bool is_adaptive() const { return adaptive_.load(std::memory_order_relaxed); }

    void set_gain(float g) {
        base_gain_.store(std::clamp(g, 0.0f, 2.0f), std::memory_order_relaxed);
        if (!adaptive_.load()) current_gain_ = base_gain_.load();
    }
    float get_gain() const { return base_gain_.load(std::memory_order_relaxed); }
    float get_current_gain() const { return current_gain_; }

    void set_gate_threshold(float t) { gate_threshold_.store(std::clamp(t, 0.0f, 0.5f), std::memory_order_relaxed); }
    float get_gate_threshold() const { return gate_threshold_.load(std::memory_order_relaxed); }

    void set_hpf_cutoff(float freq) {
        hpf_cutoff_ = std::clamp(freq, 20.0f, 500.0f);
        update_hpf_coeff();
    }
    float get_hpf_cutoff() const { return hpf_cutoff_; }

    void set_lpf_cutoff(float freq) {
        lpf_cutoff_ = std::clamp(freq, 500.0f, 8000.0f);
        update_lpf_coeff();
    }
    float get_lpf_cutoff() const { return lpf_cutoff_; }

    float get_noise_level() const { return noise_level_; }

    void reset_state() {
        hpf_prev_in_ = 0.0f;
        hpf_prev_out_ = 0.0f;
        lpf_prev_out_ = 0.0f;
        noise_level_ = 0.0f;
        current_gain_ = base_gain_.load();
        current_gate_ = gate_threshold_.load();
    }

    int get_sample_rate() const { return sample_rate_; }

private:
    void update_hpf_coeff() {
        float rc = 1.0f / (2.0f * M_PI * hpf_cutoff_);
        float dt = 1.0f / sample_rate_;
        hpf_alpha_ = rc / (rc + dt);
    }

    void update_lpf_coeff() {
        float rc = 1.0f / (2.0f * M_PI * lpf_cutoff_);
        float dt = 1.0f / sample_rate_;
        lpf_alpha_ = dt / (rc + dt);
    }

    int sample_rate_;
    float hpf_alpha_;
    float lpf_alpha_;
    float hpf_prev_in_;
    float hpf_prev_out_;
    float lpf_prev_out_;
    float hpf_cutoff_;
    float lpf_cutoff_;

    std::atomic<bool> enabled_{false};
    std::atomic<bool> adaptive_{true}; // adaptive mode ON by default
    std::atomic<float> base_gain_{0.2f};
    std::atomic<float> gate_threshold_{0.01f};

    // Adaptive state (not atomic, updated only in process thread)
    float noise_level_{0.0f};
    float current_gain_{0.2f};
    float current_gate_{0.01f};
};
