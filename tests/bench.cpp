// CPU: worst cases. 2.x: both filters at 48 dB, stretch mode crossfading, both LFOs and 24 slots running.
// 3.0: six filters at 48 dB, 15 taps, every effect (pitch in the loop), M/S, and all 50 new slots running.
#include "../plugins/Strata/ParamDefs.hpp"
#include <chrono>
#include <cstdio>
using namespace strata;
static double run(Engine& e, int secs) {
    const int SR = 48000; static float in[48000], L[48000], R[48000]; uint32_t r = 1;
    for (float& x : in) { r = r * 1664525u + 1013904223u; x = 0.3f * ((int32_t)r * (1.f / 2147483648.f)); }
    auto t0 = std::chrono::steady_clock::now();
    for (int k = 0; k < secs; ++k) for (int b = 0; b < SR; b += 256) e.process(in + b, in + b, in + b, in + b, L + b, R + b, 256);
    return std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
}
int main() {
    const int secs = 10;
    {
        Engine e; e.setSampleRate(48000);
        e.p[kF1Slope] = 2; e.p[kF2Slope] = 2; e.p[kF1Style] = 3; e.p[kF2Style] = 5; e.p[kMode] = 1; e.p[kFbL] = 0.8f; e.p[kFbR] = 0.8f; e.p[kXfLR] = 0.3f; e.p[kXfRL] = 0.3f;
        for (int s = 0; s < kNumSlots; ++s) e.setSlot(s, 1 + s % (kNumSources - 1), 1 + s % (kDstSlotFirst - 1), 0.1f);
        const double el = run(e, secs); printf("2.x worst case: %d s of audio in %.3f s: %.1f%% of one core\n", secs, el, 100 * el / secs);
    }
    {
        Engine e; e.setSampleRate(48000);
        e.p[kMode] = 1; e.p[kFbL] = 0.8f; e.p[kFbR] = 0.8f; e.p[kXfLR] = 0.3f; e.p[kXfRL] = 0.3f; e.p[kChanMode] = 1;
        for (int f = 0; f < kNumFilters; ++f) { e.p[fpar(f, kFOn)] = 1; e.p[fpar(f, kFSlope)] = kSlope48; e.p[fpar(f, kFStyle)] = 3 + f; e.p[fpar(f, kFType)] = f < 2 ? kLowPass : kBell; }
        for (int t = 0; t < kNumTaps; ++t) e.p[tpar(t, kTOn)] = 1;
        e.p[kLofiOn] = 1; e.p[kDiffOn] = 1; e.p[kDynOn] = 1; e.p[kPitchOn] = 1; e.p[kPitch] = 7; e.p[kPitchInLoop] = 1;
        for (int l = 0; l < em::kNumXlfo; ++l) e.p[lp_(l, em::kLRate)] = 0.3f + l;
        for (int s = 0; s < kNumSlots2; ++s) e.setSlot2(s, 1 + s % (em::kNumSources - 1), 1 + s % (kV2SlotFirst - 1), 0.05f);
        const double el = run(e, secs); printf("3.0 worst case: %d s of audio in %.3f s: %.1f%% of one core\n", secs, el, 100 * el / secs);
    }
    return 0;
}
