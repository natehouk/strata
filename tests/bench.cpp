// CPU: worst case — both filters at 48 dB, stretch mode crossfading, both LFOs and 24 slots running.
#include "../plugins/Strata/ParamDefs.hpp"
#include <chrono>
#include <cstdio>
using namespace strata;
int main() {
    const int SR = 48000; Engine e; e.setSampleRate(SR);
    e.p[kF1Slope] = 2; e.p[kF2Slope] = 2; e.p[kF1Style] = 3; e.p[kF2Style] = 5; e.p[kMode] = 1; e.p[kFbL] = 0.8f; e.p[kFbR] = 0.8f; e.p[kXfLR] = 0.3f; e.p[kXfRL] = 0.3f;
    for (int s = 0; s < kNumSlots; ++s) e.setSlot(s, 1 + s % (kNumSources - 1), 1 + s % (kDstSlotFirst - 1), 0.1f);
    static float in[SR], L[SR], R[SR]; uint32_t r = 1; for (float& x : in) { r = r * 1664525u + 1013904223u; x = 0.3f * ((int32_t)r * (1.f / 2147483648.f)); }
    const int secs = 20; auto t0 = std::chrono::steady_clock::now();
    for (int k = 0; k < secs; ++k) for (int b = 0; b < SR; b += 256) e.process(in + b, in + b, L + b, R + b, 256);
    const double el = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
    printf("%d s of audio in %.3f s: %.1f%% of one core\n", secs, el, 100 * el / secs); return 0;
}
