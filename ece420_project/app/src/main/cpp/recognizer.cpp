#include "recognizer.h"
#include <limits>
#include <cmath>
#include <android/log.h>
#define LOG_TAG "Native"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)

void GenderRecognizer::addExample(const std::string& label, const MFCC& mfcc) {
    db[label].push_back(mfcc);
}

float GenderRecognizer::distance(const MFCC& a, const MFCC& b) {
    float sum = 0;
    for (int i = 0; i < a.size(); ++i) {
        float diff = a[i] - b[i];
        sum += diff * diff;
    }
    return sqrtf(sum);
}

std::string GenderRecognizer::recognize(const MFCC& mfcc, int k) {

    if (db.empty()) {
        LOGD("Recognizer DB is empty — no training examples?");
    }

    std::vector<std::pair<float, std::string>> distances;
    for (const auto& [label, vecs] : db) {
        for (const auto& train : vecs) {
            float d = distance(mfcc, train);
            distances.emplace_back(d, label);
        }
    }

    std::sort(distances.begin(), distances.end(),
              [](const auto& a, const auto& b) { return a.first < b.first; });

    std::unordered_map<std::string, int> labelCounts;
    for (int i = 0; i < std::min(k, (int)distances.size()); ++i) {
        labelCounts[distances[i].second]++;
    }

    std::string bestLabel;
    int maxCount = 0;
    for (const auto& [label, count] : labelCounts) {
        if (count > maxCount) {
            maxCount = count;
            bestLabel = label;
        }
    }

    return bestLabel;
}
