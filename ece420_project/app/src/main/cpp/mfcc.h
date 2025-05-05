#pragma once
#include <vector>
#include <cmath>
#include "kissfft/kiss_fftr.h"

#define SAMPLE_RATE 16000
#define FRAME_SIZE 512
#define FRAME_SHIFT 256
#define NUM_MFCC 13
#define NUM_MEL_BANDS 26

using Frame = std::vector<float>;
using MFCC = std::vector<float>;

class MFCCExtractor {
public:
    MFCCExtractor();
    std::vector<MFCC> extract(const std::vector<short>& pcm);
private:
    std::vector<std::vector<float>> melFilterBank;
    std::vector<Frame> frameSignal(const std::vector<float>& signal);
    void applyHamming(Frame& frame);
    std::vector<std::vector<float>> createMelFilterBank(int nfft);
};
