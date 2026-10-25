// 2.x compatibility: random 2.x settings (3.0 parameters at their defaults), modulation slots, MIDI, tempo,
// changes between blocks, through the frozen 2.0 engine and the 3.0 engine: outputs must match.
#include "../plugins/Strata/ParamDefs.hpp"
#include "legacy/StrataEngine2.hpp"
#include <cstdio>
#include <memory>
#include <random>
#include <vector>
using namespace strata;
int main() {
    std::mt19937 g(9); std::uniform_real_distribution<float> u(0, 1);
    int bad = 0; float worst = -1000;
    for (int trial = 0; trial < 36; ++trial) {
        const int sr = trial % 3 == 0 ? 44100 : (trial % 3 == 1 ? 48000 : 96000);
        std::unique_ptr<Engine> e(new Engine()); std::unique_ptr<strata2::Engine> o(new strata2::Engine());
        auto rnd = [&](uint32_t i) {
            const ParamDef& d = paramDef(i); float lo = d.min, hi = d.max;
            if (i == kF1Style || i == kF2Style) hi = kNumStyles2 - 1;            // 2.x ranges
            if (i == kF1Type || i == kF2Type || i == kF1Slope || i == kF2Slope) hi = 2;
            if (i == kRouting) hi = 1;
            if (i == kInput) hi = 2;
            if (i >= kSlotFirst && i <= kSlotLast && (i - kSlotFirst) % 4 == kSlotDst) hi = strata2::kNumDests - 1;
            float v = (d.hints & kHintLog) ? lo * std::pow(hi / lo, u(g)) : lo + (hi - lo) * u(g);
            if (d.hints & kHintInteger) v = std::round(v);
            if (i >= kSlotFirst && i <= kSlotLast && (i - kSlotFirst) % 4 == kSlotSrc && u(g) < 0.6f) v = 0;
            if (i == kOutGain) v = std::min(v, 6.f);
            if (i == kFbL || i == kFbR || i == kXfLR || i == kXfRL) v *= 0.8f;
            return v;
        };
        for (uint32_t i = 0; i < kOutLearn; ++i) { const float v = rnd(i); e->p[i] = v; o->p[i] = v; }
        e->setSampleRate((float)sr); o->setSampleRate((float)sr);
        const int N = sr; std::vector<float> l(N), r(N), L1(N), R1(N), L2(N), R2(N);
        for (int i = 0; i < N; ++i) { const float env = (i / (sr / 6)) % 3 == 0 ? 0.5f : 0.0f; l[i] = env * (2 * u(g) - 1); r[i] = 0.6f * l[i] + 0.3f * env * (2 * u(g) - 1); }
        for (int i = 0; i < N; i += 300) {
            const int n = std::min(300, N - i);
            const float ev = u(g);
            if (ev < 0.05f) { const int nt = 40 + (int)(u(g) * 40), vv = 1 + (int)(u(g) * 126); e->noteOn(nt, vv); o->noteOn(nt, vv); }
            else if (ev < 0.08f) { e->allNotesOff(); o->allNotesOff(); }
            else if (ev < 0.10f) { const int w = (int)(u(g) * 127); e->modWheel(w); o->modWheel(w); }
            if (u(g) < 0.04f) { const uint32_t k = (uint32_t)(u(g) * kSlotFirst); const float v = rnd(k); e->p[k] = v; o->p[k] = v; }
            const bool play = (i / 9000) % 2 == 0;
            e->setTransport(true, play, i / (double)sr * 0.5, 120, 4); o->setTransport(true, play, i / (double)sr * 0.5, 120, 4);
            e->process(l.data() + i, r.data() + i, L1.data() + i, R1.data() + i, n);
            o->process(l.data() + i, r.data() + i, L2.data() + i, R2.data() + i, n);
        }
        double dd = 0, s = 1e-30; for (int i = 0; i < N; ++i) { dd += (L1[i] - L2[i]) * (L1[i] - L2[i]) + (R1[i] - R2[i]) * (R1[i] - R2[i]); s += l[i] * l[i] + r[i] * r[i]; }
        const float db = dd == 0 ? -240.f : (float)(10 * std::log10(dd / s));
        worst = std::max(worst, db); if (db > -120) { ++bad; printf("trial %d (%d Hz): difference %.1f dB\n", trial, sr, db); }
    }
    printf("2.x compatibility: largest difference %.1f dB below the input level, %d failures\n", -worst, bad);
    return bad;
}
