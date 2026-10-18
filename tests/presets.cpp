// Run a drum-like pulse train plus noise through every factory preset (with a held MIDI note); report levels.
#include "../plugins/Strata/ParamDefs.hpp"
#include "../plugins/Strata/Presets.hpp"
#include <cstdio>
#include <vector>
using namespace strata;
int main() {
    const int SR = 48000, N = SR * 4; int bad = 0;
    std::vector<float> x(N); uint32_t r = 3; float lp = 0;
    for (int i = 0; i < N; ++i) {
        r = r * 1664525u + 1013904223u; const float w = (int32_t)r * (1.f / 2147483648.f); lp += 0.05f * (w - lp);
        const int ph = i % (SR / 2); const float env = ph < 4800 ? std::exp(-ph / 900.f) : 0.f;
        x[i] = (i < SR * 2) ? env * (0.4f * w + 0.6f * std::sin(2 * kPi * 220 * i / (float)SR)) + 0.05f * lp : 0.f;   // 2 s of hits, then silence
    }
    double sin_ = 0; for (int i = 0; i < SR * 2; ++i) sin_ += x[i] * x[i]; const float inR = (float)std::sqrt(sin_ / (SR * 2));
    for (uint32_t p = 0; p < kNumPresets; ++p) {
        Engine e; e.setSampleRate(SR);
        for (uint32_t k = 0; k < kParamCount; ++k) e.p[k] = presets()[p].v[k];
        e.setTransport(true, true, 0, 120, 4); e.reset(); e.noteOn(60, 100); e.modWheel(64);
        std::vector<float> L(N), R(N);
        for (int i = 0; i < N; i += 512) e.process(x.data() + i, x.data() + i, L.data() + i, R.data() + i, std::min(512, N - i));
        double s = 0, t = 0; float pk = 0; bool nf = false;
        for (int i = 0; i < N; ++i) { (i < SR * 2 ? s : t) += L[i] * L[i] + R[i] * R[i]; pk = std::max(pk, std::max(std::fabs(L[i]), std::fabs(R[i]))); nf |= !std::isfinite(L[i]) || !std::isfinite(R[i]); }
        const float rel = 20 * std::log10(std::sqrt(s / (4.0 * SR)) / inR + 1e-9f), tail = 20 * std::log10(std::sqrt(t / (4.0 * SR)) / inR + 1e-9f);
        const bool ok = !nf && rel > -12 && rel < 9 && pk <= 1.f; bad += !ok;
        printf("%-18s %-13s level %+6.1f dB  tail %+6.1f dB  peak %.2f %s\n", presets()[p].name, presets()[p].category, rel, tail, pk, ok ? "" : "<-- CHECK");
    }
    printf("%u presets, %d flagged\n", kNumPresets, bad); return bad;
}
