package com.anc.app

import android.content.Context

class NoiseReductionEngine(context: Context) {
    private var nativeHandle: Long = 0
    private val sampleRate = 48000

    init {
        nativeHandle = nativeCreate(sampleRate, 128)
    }

    fun setEnabled(enabled: Boolean) {
        if (nativeHandle != 0L) nativeSetEnabled(nativeHandle, enabled)
    }

    fun setAdaptive(adaptive: Boolean) {
        if (nativeHandle != 0L) nativeSetAdaptive(nativeHandle, adaptive)
    }

    fun setGain(gain: Float) {
        if (nativeHandle != 0L) nativeSetGain(nativeHandle, gain)
    }

    fun setGateThreshold(threshold: Float) {
        if (nativeHandle != 0L) nativeSetGateThreshold(nativeHandle, threshold)
    }

    fun setHPFCutoff(cutoff: Float) {
        if (nativeHandle != 0L) nativeSetHPFCutoff(nativeHandle, cutoff)
    }

    fun setLPFCutoff(cutoff: Float) {
        if (nativeHandle != 0L) nativeSetLPFCutoff(nativeHandle, cutoff)
    }

    fun getNoiseLevel(): Float {
        return if (nativeHandle != 0L) nativeGetNoiseLevel(nativeHandle) else 0f
    }

    fun getCurrentGain(): Float {
        return if (nativeHandle != 0L) nativeGetCurrentGain(nativeHandle) else 0f
    }

    fun reset() {
        if (nativeHandle != 0L) nativeReset(nativeHandle)
    }

    fun destroy() {
        if (nativeHandle != 0L) {
            nativeDestroy(nativeHandle)
            nativeHandle = 0
        }
    }

    protected fun finalize() {
        destroy()
    }

    private external fun nativeCreate(sampleRate: Int, filterTaps: Int): Long
    private external fun nativeDestroy(handle: Long)
    private external fun nativeSetEnabled(handle: Long, enabled: Boolean)
    private external fun nativeSetAdaptive(handle: Long, adaptive: Boolean)
    private external fun nativeSetGain(handle: Long, gain: Float)
    private external fun nativeSetGateThreshold(handle: Long, threshold: Float)
    private external fun nativeSetHPFCutoff(handle: Long, cutoff: Float)
    private external fun nativeSetLPFCutoff(handle: Long, cutoff: Float)
    private external fun nativeGetNoiseLevel(handle: Long): Float
    private external fun nativeGetCurrentGain(handle: Long): Float
    private external fun nativeReset(handle: Long)

    companion object {
        init {
            System.loadLibrary("anc")
        }
    }
}
