#include "noise_reducer.h"
#include <jni.h>

extern "C" {

JNIEXPORT jlong JNICALL
Java_com_anc_app_NoiseReductionEngine_nativeCreate(JNIEnv*, jobject, jint sample_rate) {
    auto* proc = new NoiseReducer(sample_rate);
    return reinterpret_cast<jlong>(proc);
}

JNIEXPORT void JNICALL
Java_com_anc_app_NoiseReductionEngine_nativeDestroy(JNIEnv*, jobject, jlong handle) {
    delete reinterpret_cast<NoiseReducer*>(handle);
}

JNIEXPORT void JNICALL
Java_com_anc_app_NoiseReductionEngine_nativeSetEnabled(JNIEnv*, jobject, jlong handle, jboolean enabled) {
    reinterpret_cast<NoiseReducer*>(handle)->set_enabled(enabled);
}

JNIEXPORT void JNICALL
Java_com_anc_app_NoiseReductionEngine_nativeSetGain(JNIEnv*, jobject, jlong handle, jfloat gain) {
    reinterpret_cast<NoiseReducer*>(handle)->set_gain(gain);
}

JNIEXPORT void JNICALL
Java_com_anc_app_NoiseReductionEngine_nativeSetGateThreshold(JNIEnv*, jobject, jlong handle, jfloat threshold) {
    reinterpret_cast<NoiseReducer*>(handle)->set_gate_threshold(threshold);
}

JNIEXPORT void JNICALL
Java_com_anc_app_NoiseReductionEngine_nativeSetHPFCutoff(JNIEnv*, jobject, jlong handle, jfloat cutoff) {
    reinterpret_cast<NoiseReducer*>(handle)->set_hpf_cutoff(cutoff);
}

JNIEXPORT void JNICALL
Java_com_anc_app_NoiseReductionEngine_nativeSetLPFCutoff(JNIEnv*, jobject, jlong handle, jfloat cutoff) {
    reinterpret_cast<NoiseReducer*>(handle)->set_lpf_cutoff(cutoff);
}

JNIEXPORT void JNICALL
Java_com_anc_app_NoiseReductionEngine_nativeSetAdaptive(JNIEnv*, jobject, jlong handle, jboolean adaptive) {
    reinterpret_cast<NoiseReducer*>(handle)->set_adaptive(adaptive);
}

JNIEXPORT jfloat JNICALL
Java_com_anc_app_NoiseReductionEngine_nativeGetNoiseLevel(JNIEnv*, jobject, jlong handle) {
    return reinterpret_cast<NoiseReducer*>(handle)->get_noise_level();
}

JNIEXPORT jfloat JNICALL
Java_com_anc_app_NoiseReductionEngine_nativeGetCurrentGain(JNIEnv*, jobject, jlong handle) {
    return reinterpret_cast<NoiseReducer*>(handle)->get_current_gain();
}

JNIEXPORT void JNICALL
Java_com_anc_app_NoiseReductionEngine_nativeReset(JNIEnv*, jobject, jlong handle) {
    reinterpret_cast<NoiseReducer*>(handle)->reset_state();
}

} // extern "C"
