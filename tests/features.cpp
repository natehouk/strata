// Behaviour checks: echo timing, sync/offset/link, feedback, cross-feedback, invert, pan, tape vs stretch,
// filters in the loop, modulation (LFO, EG, slot-on-slot), runaway safety.
#include "../plugins/Strata/ParamDefs.hpp"
#include <cstdio>
#include <vector>
using namespace strata;
static int fails = 0;
#define CHECK(c, msg) do { bool ok_ = (c); printf("%s  %s\n", ok_ ? "PASS" : "FAIL", msg); if (!ok_) fails++; } while (0)
static const int SR = 48000;
struct Buf { std::vector<float> l, r; };
static Buf run(Engine& e, const std::vector<float>& xl, const std::vector<float>& xr) {
    Buf b; b.l.resize(xl.size()); b.r.resize(xl.size());
    for (size_t i = 0; i < xl.size(); i += 256) { const uint32_t n = (uint32_t)std::min<size_t>(256, xl.size() - i); e.process(xl.data() + i, xr.data() + i, b.l.data() + i, b.r.data() + i, n); }
    return b;
}
static Buf run(Engine& e, const std::vector<float>& x) { return run(e, x, x); }
static float rms(const std::vector<float>& v, size_t a = 0, size_t z = 0) { if (!z) z = v.size(); double s = 0; for (size_t i = a; i < z; ++i) s += v[i] * v[i]; return (float)std::sqrt(s / (z - a)); }
static std::vector<float> sine(int n, float f, float amp) { std::vector<float> v(n); for (int i = 0; i < n; ++i) v[i] = amp * std::sin(2 * kPi * f * i / SR); return v; }
// index and value of the largest |sample| in [a, z)
static int peakAt(const std::vector<float>& v, int a, int z, float* val = nullptr) {
    int best = a; for (int i = a; i < z; ++i) if (std::fabs(v[i]) > std::fabs(v[best])) best = i; if (val) *val = v[best]; return best;
}
// a plain delay: filters and drive off, full wet, hard-panned lines
static Engine plain() {
    Engine e; e.setSampleRate(SR);
    e.p[kF1On] = 0; e.p[kF2On] = 0; e.p[kDrive] = 0; e.p[kMix] = 1; e.p[kPanL] = -1; e.p[kPanR] = 1;
    e.p[kSyncL] = 0; e.p[kSyncR] = 0; e.p[kFbL] = 0; e.p[kFbR] = 0; e.p[kGlide] = 0.001f;
    e.reset(); return e;
}
static std::vector<float> impulse(int n, float amp = 0.5f) { std::vector<float> x(n, 0.f); x[100] = amp; return x; }
// average frequency of a signal from its zero crossings in [a, z)
static float zcFreq(const std::vector<float>& v, int a, int z) {
    int n = 0, first = -1, last = -1;
    for (int i = a + 1; i < z; ++i) if (v[i - 1] < 0 && v[i] >= 0) { if (first < 0) first = i; last = i; n++; }
    return n > 1 ? (n - 1) * (float)SR / (last - first) : 0.f;
}

int main() {
    char msg[160];
    { // free times, hard-panned lines
        Engine e = plain(); e.p[kTimeL] = 300; e.p[kTimeR] = 450; e.reset();
        Buf b = run(e, impulse(SR));
        const int pl = peakAt(b.l, 200, SR), pr = peakAt(b.r, 200, SR);
        snprintf(msg, sizeof msg, "free time: left echo at %.1f ms, right at %.1f ms", (pl - 100) * 1000.f / SR, (pr - 100) * 1000.f / SR);
        CHECK(std::abs(pl - 100 - 300 * 48) <= 2 && std::abs(pr - 100 - 450 * 48) <= 2, msg);
        CHECK(rms(b.r, 100 + 300 * 48 - 50, 100 + 300 * 48 + 50) < 1e-4f, "pan: hard-left line is absent from the right output");
    }
    { // sync: 1/4 straight, 1/8 dotted, 1/4 triplet, offset, at 120 BPM
        struct T { int sync, div; float off, ms; const char* nm; } ts[] = {
            { 1, 4, 0, 500, "1/4" }, { 2, 3, 0, 375, "1/8 dotted" }, { 3, 4, 0, 333.33f, "1/4 triplet" }, { 1, 4, 10, 550, "1/4 +10% offset" }, { 1, 2, -20, 100, "1/16 -20% offset" } };
        for (const T& t : ts) {
            Engine e = plain(); e.p[kSyncL] = t.sync; e.p[kDivL] = t.div; e.p[kOffL] = t.off; e.setTransport(true, false, 0, 120, 4); e.reset();
            Buf b = run(e, impulse(SR));
            const float got = (peakAt(b.l, 200, SR) - 100) * 1000.f / SR;
            snprintf(msg, sizeof msg, "sync %-16s = %.1f ms (expect %.1f)", t.nm, got, t.ms); CHECK(std::fabs(got - t.ms) < 0.1f, msg);
        }
        Engine e = plain(); e.p[kSyncL] = 1; e.p[kDivL] = 4; e.setTransport(true, true, 0, 90, 4); e.reset();
        Buf b = run(e, impulse(SR));
        const float got = (peakAt(b.l, 200, SR) - 100) * 1000.f / SR;
        snprintf(msg, sizeof msg, "sync follows host tempo: 1/4 at 90 BPM = %.1f ms", got); CHECK(std::fabs(got - 666.67f) < 0.1f, msg);
    }
    { // link: right follows left
        Engine e = plain(); e.p[kTimeL] = 200; e.p[kTimeR] = 700; e.p[kLink] = 1; e.reset();
        Buf b = run(e, impulse(SR));
        CHECK(std::abs(peakAt(b.r, 200, SR) - 100 - 200 * 48) <= 2, "link: right line uses the left time");
    }
    { // feedback: echo n+1 = feedback x echo n
        Engine e = plain(); e.p[kTimeL] = 100; e.p[kFbL] = 0.5f; e.reset();
        Buf b = run(e, impulse(SR));
        float a1, a2, a3; peakAt(b.l, 4800 + 50, 4800 + 150, &a1); peakAt(b.l, 9600 + 50, 9600 + 150, &a2); peakAt(b.l, 14400 + 50, 14400 + 150, &a3);
        snprintf(msg, sizeof msg, "feedback 50%%: echoes %.3f %.3f %.3f", a1, a2, a3);
        CHECK(std::fabs(a2 / a1 - 0.5f) < 0.02f && std::fabs(a3 / a2 - 0.5f) < 0.02f, msg);
    }
    { // invert: the fed-back echo flips sign
        Engine e = plain(); e.p[kTimeL] = 100; e.p[kFbL] = 0.5f; e.p[kInvL] = 1; e.reset();
        Buf b = run(e, impulse(SR));
        float a1, a2; peakAt(b.l, 4850, 4950, &a1); peakAt(b.l, 9650, 9750, &a2);
        snprintf(msg, sizeof msg, "invert: echo 1 %+.3f, echo 2 %+.3f", a1, a2); CHECK(a1 * a2 < 0, msg);
    }
    { // cross-feedback: left-only input reaches the right line after one left echo (ping-pong)
        Engine e = plain(); e.p[kTimeL] = 100; e.p[kTimeR] = 150; e.p[kXfLR] = 0.8f; e.reset();
        std::vector<float> xl = impulse(SR), xr(SR, 0.f);
        Buf b = run(e, xl, xr);
        float v; const int pr = peakAt(b.r, 200, SR, &v);
        snprintf(msg, sizeof msg, "cross L>R: right echo at %.1f ms, level %.2f", (pr - 100) * 1000.f / SR, v);
        CHECK(std::abs(pr - 100 - 250 * 48) <= 3 && std::fabs(v - 0.4f) < 0.03f, msg);
    }
    { // ping-pong input: the sum enters the left line only, then bounces right
        Engine e = plain(); e.p[kInput] = 2; e.p[kTimeL] = 100; e.p[kTimeR] = 100; e.p[kXfLR] = 0.6f; e.p[kXfRL] = 0.6f; e.reset();
        Buf b = run(e, impulse(SR));
        const int l1 = peakAt(b.l, 200, 7000), r1 = peakAt(b.r, 200, 12000), l2 = peakAt(b.l, 7000, 16000);
        snprintf(msg, sizeof msg, "ping-pong: L %.0f ms, R %.0f ms, L %.0f ms", (l1 - 100) / 48.f, (r1 - 100) / 48.f, (l2 - 100) / 48.f);
        CHECK(std::abs(l1 - 4900) <= 2 && std::abs(r1 - 9700) <= 2 && std::abs(l2 - 14500) <= 2 && rms(b.r, 4800, 5000) < 1e-4f, msg);
    }
    { // tape mode bends pitch while the time changes; stretch mode does not
        for (int mode = 0; mode < 2; ++mode) {
            Engine e = plain(); e.p[kMode] = mode; e.p[kTimeL] = 400; e.p[kTimeR] = 400; e.p[kGlide] = 0.5f; e.reset();
            auto x = sine(SR * 3, 1000, 0.3f);
            Buf b1 = run(e, std::vector<float>(x.begin(), x.begin() + SR));
            e.p[kTimeL] = 200;   // shorten the delay: tape head runs faster -> pitch rises
            Buf b2 = run(e, std::vector<float>(x.begin() + SR, x.end()));
            const float during = zcFreq(b2.l, 2400, 7200), after = zcFreq(b2.l, SR, 2 * SR);
            snprintf(msg, sizeof msg, "%s mode: %.0f Hz while the time changes, %.0f Hz after", mode ? "stretch" : "tape", during, after);
            CHECK(mode == 0 ? (during > 1100 && std::fabs(after - 1000) < 5) : (std::fabs(during - 1000) < 15 && std::fabs(after - 1000) < 5), msg);
        }
    }
    { // tape mode: LFO on time = vibrato (chorus); stretch mode keeps the pitch
        Engine e = plain(); e.p[kTimeL] = 20; e.p[kL1Rate] = 2; e.p[kL1Wave] = 1; e.setSlot(0, kSrcLfo1, kDstTimeL, 0.06f); e.reset();
        Buf b = run(e, sine(SR * 2, 1000, 0.3f));
        float lo = 1e9f, hi = 0; for (int k = 0; k < 16; ++k) { const float f = zcFreq(b.l, SR / 2 + k * 2400, SR / 2 + (k + 1) * 2400); lo = std::min(lo, f); hi = std::max(hi, f); }
        snprintf(msg, sizeof msg, "LFO 1 > left time: chorus vibrato %.0f..%.0f Hz", lo, hi); CHECK(hi - lo > 20, msg);
    }
    { // filters in the loop: each repeat gets darker
        Engine e = plain(); e.p[kF1On] = 1; e.p[kF1Type] = 0; e.p[kF1Freq] = 1000; e.p[kF1Slope] = 1; e.p[kF1Peak] = 0; e.p[kTimeL] = 100; e.p[kFbL] = 0.9f; e.reset();
        auto x = sine(SR, 3000, 0.4f); for (int i = 0; i < SR; ++i) x[i] *= i < 2400 ? 0.5f - 0.5f * std::cos(2 * kPi * i / 2400.f) : 0.f;   // 50 ms Hann burst
        Buf b = run(e, x);
        const float e1 = rms(b.l, 4800, 7200), e2 = rms(b.l, 9600, 12000), e3 = rms(b.l, 14400, 16800);
        snprintf(msg, sizeof msg, "LP 1 kHz in the loop: echo levels %.4f %.4f %.4f", e1, e2, e3);
        CHECK(e1 < 0.4f * 0.2f && e2 < e1 * 0.2f && e3 < e2 * 0.2f, msg);
        Engine p = plain(); p.p[kF1On] = 1; p.p[kF2On] = 1; p.p[kF1Freq] = 500; p.p[kF2Type] = 2; p.p[kF2Freq] = 3000; p.p[kF1Peak] = 0; p.p[kF2Peak] = 0; p.p[kTimeL] = 10; p.reset();
        Engine q = p; q.p[kRouting] = 1;
        const float rs = rms(run(p, sine(SR, 1200, 0.3f)).l, SR / 2), rp = rms(run(q, sine(SR, 1200, 0.3f)).l, SR / 2);
        snprintf(msg, sizeof msg, "routing: serial LP+HP %.4f vs parallel %.4f", rs, rp); CHECK(rp > rs * 3, msg);
    }
    { // filter pan: different cutoff per side
        Engine e = plain(); e.p[kF1On] = 1; e.p[kF1Freq] = 1000; e.p[kF1Peak] = 0; e.p[kF1Pan] = 1; e.p[kTimeL] = 10; e.p[kTimeR] = 10; e.reset();
        Buf b = run(e, sine(SR, 2500, 0.3f));
        CHECK(rms(b.r, SR / 2) > 2 * rms(b.l, SR / 2), "filter pan: right side brighter at pan +1");
    }
    { // self-oscillating filter styles stay bounded with 110% feedback
        bool ok = true; float worst = 0;
        for (int st = 0; st < kNumStyles; ++st) {
            Engine e = plain(); e.p[kF1On] = 1; e.p[kF1Style] = st; e.p[kF1Peak] = 1; e.p[kFbL] = 1.1f; e.p[kFbR] = 1.1f; e.p[kXfLR] = 1.1f; e.p[kXfRL] = 1.1f; e.p[kTimeL] = 30; e.p[kTimeR] = 7; e.reset();
            Buf b = run(e, impulse(SR * 4, 1.f));
            for (int i = 0; i < SR * 4; ++i) { if (!std::isfinite(b.l[i]) || std::fabs(b.l[i]) > 1.f) ok = false; worst = std::max(worst, std::fabs(b.l[i])); }
        }
        snprintf(msg, sizeof msg, "runaway: every style at 110%% feedback + cross stays finite (peak %.2f)", worst); CHECK(ok, msg);
    }
    { // drive saturates repeats
        Engine e = plain(); e.p[kTimeL] = 50; e.p[kFbL] = 0.95f; e.p[kDrive] = 1; e.reset();
        Buf b = run(e, sine(SR * 2, 200, 0.8f));
        CHECK(rms(b.l, SR) < 0.9f && rms(b.l, SR) > 0.1f, "drive: saturated loop at 95% feedback is loud but bounded");
    }
    { // EG: audio trigger above threshold; MIDI trigger by notes; drives the mix
        Engine e = plain(); e.p[kEgThreshold] = -20; e.p[kEgAttack] = 0.001f; e.p[kEgSustain] = 1; e.reset();
        run(e, sine(SR / 4, 300, 0.02f)); const float quiet = e.egValue();
        run(e, sine(SR / 4, 300, 0.5f)); const float loud = e.egValue();
        snprintf(msg, sizeof msg, "EG audio trigger: %.2f below threshold, %.2f above", quiet, loud); CHECK(quiet < 0.01f && loud > 0.9f, msg);
        Engine m = plain(); m.p[kEgTrigger] = 1; m.p[kEgAttack] = 0.001f; m.p[kEgSustain] = 1; m.p[kEgRelease] = 0.01f; m.reset();
        run(m, sine(SR / 4, 300, 0.5f)); const float before = m.egValue();
        m.noteOn(60, 100); run(m, sine(SR / 10, 300, 0.f)); const float on = m.egValue();
        m.noteOff(60); run(m, sine(SR / 4, 300, 0.f)); const float off = m.egValue();
        snprintf(msg, sizeof msg, "EG MIDI trigger: %.2f before, %.2f after note-on, %.2f after note-off", before, on, off); CHECK(before < 0.01f && on > 0.9f && off < 0.01f, msg);
        Engine d = plain(); d.p[kMix] = 0; d.p[kTimeL] = 10; d.p[kEgTrigger] = 1; d.p[kEgAttack] = 0.001f; d.p[kEgSustain] = 1; d.setSlot(0, kSrcEg, kDstMix, 1.f); d.reset();
        auto x = sine(SR / 2, 450, 0.3f);
        Buf b0 = run(d, x); d.noteOn(60, 100); Buf b1 = run(d, x);
        float diff0 = 0, diff1 = 0; for (int i = SR / 4; i < SR / 2; ++i) { diff0 = std::max(diff0, std::fabs(b0.l[i] - x[i])); diff1 = std::max(diff1, std::fabs(b1.l[i] - x[i])); }
        CHECK(diff0 < 0.01f && diff1 > 0.1f, "EG > mix: dry until a note, then wet");
    }
    { // MIDI sources and slot-on-slot: mod wheel scales an LFO's depth on the delay time
        Engine e = plain(); e.p[kTimeL] = 20; e.p[kL1Rate] = 3; e.p[kL1Wave] = 1;
        e.setSlot(0, kSrcLfo1, kDstTimeL, 0.f); e.setSlot(1, kSrcWheel, kDstSlotFirst + 0, 0.08f); e.reset();
        auto spread = [&](Engine& en) { Buf b = run(en, sine(SR, 1000, 0.3f)); float lo = 1e9f, hi = 0;
            for (int k = 0; k < 8; ++k) { const float f = zcFreq(b.l, SR / 4 + k * 2400, SR / 4 + (k + 1) * 2400); lo = std::min(lo, f); hi = std::max(hi, f); } return hi - lo; };
        const float s0 = spread(e); e.modWheel(127); const float s1 = spread(e);
        snprintf(msg, sizeof msg, "slot-on-slot: wheel 0 vibrato %.1f Hz, wheel max %.1f Hz", s0, s1); CHECK(s0 < 3 && s1 > 20, msg);
        Engine k = plain(); k.p[kTimeL] = 100; k.setSlot(0, kSrcKey, kDstTimeL, 0.5f); k.reset(); k.noteOn(72, 100);
        run(k, sine(SR / 4, 100, 0.f));
        snprintf(msg, sizeof msg, "key track > time: C5 at 50%% scales 100 ms to %.1f ms (expect 141.4)", k.liveTimeMs(0)); CHECK(std::fabs(k.liveTimeMs(0) - 141.4f) < 1.f, msg);
    }
    { // bypassed slot does nothing; LFO sync locks to song position
        Engine e = plain(); e.p[kTimeL] = 100; e.setSlot(0, kSrcWheel, kDstTimeL, 1.f); e.p[slotParam(0, kSlotBypass)] = 1; e.modWheel(127); e.reset();
        run(e, sine(SR / 8, 100, 0.f)); CHECK(std::fabs(e.liveTimeMs(0) - 100) < 0.5f, "bypass: a bypassed slot has no effect");
        Engine a = plain(), c = plain();
        for (Engine* x : { &a, &c }) { x->p[kL1Sync] = 1; x->p[kL1Div] = 4; x->setSlot(0, kSrcLfo1, kDstTimeL, 0.3f); x->p[kTimeL] = 100; x->p[kGlide] = 0.001f; }
        a.setTransport(true, true, 7.25, 120, 4); c.setTransport(true, true, 3.25, 90, 4);
        run(a, sine(64, 100, 0)); run(c, sine(64, 100, 0));
        snprintf(msg, sizeof msg, "LFO sync: same bar phase gives the same time (%.2f vs %.2f ms)", a.liveTimeMs(0), c.liveTimeMs(0));
        CHECK(std::fabs(a.liveTimeMs(0) - c.liveTimeMs(0)) < 1.f, msg);
    }
    { // mix 0 is the dry signal; longest synced note at a slow tempo clamps to 4 s
        Engine e = plain(); e.p[kMix] = 0; e.p[kFbL] = 0.9f; e.reset(); auto x = sine(SR, 3000, 0.5f); Buf b = run(e, x);
        float d = 0; for (int i = SR / 4; i < SR; ++i) d = std::max(d, std::fabs(b.l[i] - x[i])); CHECK(d < 2e-3f, "mix 0%: output equals input");
        Engine s; s.setSampleRate(192000); s.p[kSyncL] = 2; s.p[kDivL] = 7; s.setTransport(true, false, 0, 40, 4); s.reset();
        std::vector<float> z(4096, 0.f), L(4096), R(4096); s.process(z.data(), z.data(), L.data(), R.data(), 4096);
        snprintf(msg, sizeof msg, "2 bars dotted at 40 BPM clamps to %.0f ms", s.liveTimeMs(0)); CHECK(std::fabs(s.liveTimeMs(0) - kMaxDelayMs) < 1, msg);
    }
    printf("%s (%d failures)\n", fails ? "FAILED" : "ALL PASSED", fails);
    return fails ? 1 : 0;
}
