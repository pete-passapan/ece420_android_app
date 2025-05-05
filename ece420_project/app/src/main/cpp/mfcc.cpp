#include "mfcc.h"
#include <algorithm>

static float hzToMel(float hz) {
    return 2595.0f * log10f(1.0f + hz / 700.0f);
}

static float melToHz(float mel) {
    return 700.0f * (powf(10.0f, mel / 2595.0f) - 1.0f);
}

MFCCExtractor::MFCCExtractor() {
    melFilterBank = createMelFilterBank(FRAME_SIZE);
}

std::vector<MFCC> MFCCExtractor::extract(const std::vector<short>& pcm) {
    std::vector<float> floatSignal(pcm.begin(), pcm.end());
    auto frames = frameSignal(floatSignal);
    std::vector<MFCC> mfccs;
    kiss_fftr_cfg fftCfg = kiss_fftr_alloc(FRAME_SIZE, false, 0, 0);

    for (auto& frame : frames) {
        applyHamming(frame);
        std::vector<kiss_fft_scalar> in(FRAME_SIZE);
        std::vector<kiss_fft_cpx> out(FRAME_SIZE / 2 + 1);
        std::copy(frame.begin(), frame.end(), in.begin());
        kiss_fftr(fftCfg, in.data(), out.data());

        std::vector<float> power(FRAME_SIZE / 2 + 1);
        for (int i = 0; i <= FRAME_SIZE / 2; ++i)
            power[i] = out[i].r * out[i].r + out[i].i * out[i].i;

        std::vector<float> melEnergies(NUM_MEL_BANDS);
        for (int m = 0; m < NUM_MEL_BANDS; ++m) {
            for (int k = 0; k <= FRAME_SIZE / 2; ++k)
                melEnergies[m] += power[k] * melFilterBank[m][k];
            melEnergies[m] = logf(melEnergies[m] + 1e-6f);
        }

        MFCC coeffs(NUM_MFCC, 0.0f);
        for (int k = 0; k < NUM_MFCC; ++k) {
            for (int m = 0; m < NUM_MEL_BANDS; ++m)
                coeffs[k] += melEnergies[m] * cosf(M_PI * k * (m + 0.5f) / NUM_MEL_BANDS);
        }

        mfccs.push_back(coeffs);
    }

    kiss_fftr_free(fftCfg);

    return mfccs;
}

std::vector<Frame> MFCCExtractor::frameSignal(const std::vector<float>& signal) {
    std::vector<Frame> frames;
    for (size_t i = 0; i + FRAME_SIZE <= signal.size(); i += FRAME_SHIFT)
        frames.emplace_back(signal.begin() + i, signal.begin() + i + FRAME_SIZE);
    return frames;
}

void MFCCExtractor::applyHamming(Frame& frame) {
    for (int i = 0; i < FRAME_SIZE; ++i)
        frame[i] *= 0.54f - 0.46f * cosf(2.0f * M_PI * i / (FRAME_SIZE - 1));
}

std::vector<std::vector<float>> MFCCExtractor::createMelFilterBank(int nfft) {
    int numBins = nfft / 2 + 1;
    std::vector<std::vector<float>> filters(NUM_MEL_BANDS, std::vector<float>(numBins, 0.0f));
    float melLow = hzToMel(0);
    float melHigh = hzToMel(SAMPLE_RATE / 2);

    std::vector<float> melPoints(NUM_MEL_BANDS + 2);
    for (int i = 0; i < melPoints.size(); ++i)
        melPoints[i] = melLow + (melHigh - melLow) * i / (NUM_MEL_BANDS + 1);

    std::vector<float> hzPoints(melPoints.size());
    for (int i = 0; i < melPoints.size(); ++i)
        hzPoints[i] = melToHz(melPoints[i]);

    std::vector<int> binPoints(hzPoints.size());
    for (int i = 0; i < binPoints.size(); ++i)
        binPoints[i] = static_cast<int>(hzPoints[i] * nfft / SAMPLE_RATE);

    for (int m = 1; m <= NUM_MEL_BANDS; ++m) {
        int f_m_minus = binPoints[m - 1];
        int f_m = binPoints[m];
        int f_m_plus = binPoints[m + 1];
        for (int k = f_m_minus; k < f_m; ++k)
            filters[m - 1][k] = static_cast<float>(k - f_m_minus) / (f_m - f_m_minus);
        for (int k = f_m; k < f_m_plus; ++k)
            filters[m - 1][k] = static_cast<float>(f_m_plus - k) / (f_m_plus - f_m);
    }

    return filters;
}
