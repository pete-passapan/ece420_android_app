#include <jni.h>
#include <string>
#include <vector>
#include "mfcc.h"
#include "recognizer.h"
#include <android/log.h>
#define LOG_TAG "Native"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)

static MFCCExtractor extractor;
static GenderRecognizer recognizer;

// For demo: hardcoded training data (in practice, you'd load or record these)
extern "C"
JNIEXPORT void JNICALL Java_com_example_ece420_1project_MainActivity_addTrainingExample(
        JNIEnv* env, jobject /* this */,
        jstring label, jshortArray audioData) {

    const char* labelStr = env->GetStringUTFChars(label, nullptr);
    jsize len = env->GetArrayLength(audioData);
    std::vector<short> pcm(len);
    env->GetShortArrayRegion(audioData, 0, len, pcm.data());
    LOGD("Adding training example for: %s, len=%d", labelStr, len);

    auto mfccFrames = extractor.extract(pcm);
    if (mfccFrames.empty()) {
        LOGD("MFCC extraction returned empty.");
    } else {
        LOGD("Extracted %d MFCC frames", (int)mfccFrames.size());
    }

    if (!mfccFrames.empty()) {
        // Average across frames
        MFCC avg(NUM_MFCC, 0.0f);
        for (const auto& frame : mfccFrames) {
            for (int i = 0; i < NUM_MFCC; ++i) {
                avg[i] += frame[i];
            }
        }
        for (int i = 0; i < NUM_MFCC; ++i)
            avg[i] /= mfccFrames.size();

        recognizer.addExample(labelStr, avg);
    }

    env->ReleaseStringUTFChars(label, labelStr);
}


extern "C" JNIEXPORT jstring JNICALL
Java_com_example_ece420_1project_MainActivity_predictGender(
        JNIEnv* env, jobject /* this */, jshortArray audioData) {

    jsize len = env->GetArrayLength(audioData);
    LOGD("Predicting gender on %d samples", len);
    std::vector<short> pcm(len);
    env->GetShortArrayRegion(audioData, 0, len, pcm.data());

    float energy = 0.0f;
    for (short sample : pcm)
        energy += sample * sample;

    energy /= len; // Average energy
    const float SILENCE_THRESHOLD = 500.0f;  // Tune this!

    if (energy < SILENCE_THRESHOLD) {
        LOGD("Silence detected (energy = %f), skipping prediction.", energy);
        return env->NewStringUTF("silence");
    }


    auto mfcc = extractor.extract(pcm);
    if (mfcc.empty()) return env->NewStringUTF("unknown");

    MFCC avg(NUM_MFCC, 0.0f);

    if (!mfcc.empty()) {
        // Average across frames

        for (const auto& frame : mfcc) {
            for (int i = 0; i < NUM_MFCC; ++i) {
                avg[i] += frame[i];
            }
        }
        for (int i = 0; i < NUM_MFCC; ++i)
            avg[i] /= mfcc.size();
    }

    std::string result = recognizer.recognize(avg,10);
    LOGD("Predicted: %s", result.c_str());
    return env->NewStringUTF(result.c_str());
}