// Strata — stereo tape-style delay engine with filters in the feedback loop and a 24-slot modulation matrix.
// Clean-room implementation from public, generic techniques: circular delay lines with cubic Hermite
// interpolation, a slewed read head ("tape": time changes bend pitch) or crossfaded read heads
// ("stretch": time changes without pitch change), cascaded TPT state-variable filters (Zavalishin)
// with per-style nonlinearities. No third-party product code, assets or disassembly were used.
// License: MIT (see LICENSE).

#pragma once
#include <cmath>
#include <cstdint>
#include <algorithm>
#include <vector>

namespace strata2 {

static constexpr float kPi = 3.14159265358979f;
inline float clampf(float x, float lo, float hi) { return x < lo ? lo : (x > hi ? hi : x); }
inline float dbToGain(float db) { return std::pow(10.f, db / 20.f); }

static constexpr int kNumSlots = 24;
static constexpr float kMinDelayMs = 1.f, kMaxDelayMs = 4000.f;
static constexpr int kCtl = 8;                          // modulation is evaluated every kCtl samples

// ------------------------------------------------------------------ params (append-only)
#define FLT(n)  kF##n##On, kF##n##Style, kF##n##Type, kF##n##Slope, kF##n##Freq, kF##n##Peak, kF##n##Pan
#define LFOP(n) kL##n##Wave, kL##n##Balance, kL##n##Sync, kL##n##Rate, kL##n##Div, kL##n##NoteSync
enum Param : uint32_t {
    kTimeL, kTimeR, kSyncL, kSyncR, kDivL, kDivR, kOffL, kOffR, kLink, kMode, kGlide,
    kPanL, kPanR, kFbL, kFbR, kXfLR, kXfRL, kInvL, kInvR, kDrive, kInput,
    FLT(1), FLT(2), kRouting,
    kInGain, kMix, kOutGain,
    LFOP(1), LFOP(2),
    kEgAttack, kEgDecay, kEgSustain, kEgRelease, kEgThreshold, kEgTrigger,
    kSlotFirst,
    kSlotLast = kSlotFirst + kNumSlots * 4 - 1,
    kMidiOn,
    // outputs (read-only, for the UI)
    kOutLearn, kOutTimeL, kOutTimeR, kOutEg,
    kParamCount
};
#undef FLT
#undef LFOP
enum SlotField { kSlotSrc = 0, kSlotDst = 1, kSlotAmt = 2, kSlotBypass = 3 };
inline uint32_t slotParam(int slot, int field) { return kSlotFirst + slot * 4 + field; }

enum Source { kSrcNone, kSrcLfo1, kSrcLfo2, kSrcEg, kSrcVelocity, kSrcAftertouch, kSrcKey, kSrcBend, kSrcWheel, kNumSources };

enum Dest {
    kDstNone,
    kDstTimeL, kDstTimeR, kDstPanL, kDstPanR, kDstFbL, kDstFbR, kDstXfLR, kDstXfRL, kDstDrive,
    kDstF1Freq, kDstF1Peak, kDstF1Pan, kDstF2Freq, kDstF2Peak, kDstF2Pan,
    kDstInGain, kDstMix, kDstOutGain,
    kDstL1Rate, kDstL1Balance, kDstL2Rate, kDstL2Balance,
    kDstEgAttack, kDstEgDecay, kDstEgSustain, kDstEgRelease,
    kDstSlotFirst,
    kNumDests = kDstSlotFirst + kNumSlots
};

// delay note values, in whole notes
static constexpr int kNumNoteDivs = 8;
inline float noteWhole(int i) {
    static const float t[kNumNoteDivs] = { 1.f/64, 1.f/32, 1.f/16, 1.f/8, 1.f/4, 1.f/2, 1.f, 2.f };
    return t[i < 0 ? 0 : (i >= kNumNoteDivs ? kNumNoteDivs - 1 : i)];
}
// LFO divisions, in bars
static constexpr int kNumDivs = 11;
inline float divBars(int i) {
    static const float t[kNumDivs] = { 16, 8, 4, 2, 1, 1.f/2, 1.f/4, 1.f/8, 1.f/16, 1.f/32, 1.f/64 };
    return t[i < 0 ? 0 : (i >= kNumDivs ? kNumDivs - 1 : i)];
}
// sync modes, shared by the delay lines and the LFOs: 0 off, 1 straight, 2 dotted, 3 triplet
inline float syncFactor(int s) { return s == 2 ? 1.5f : (s == 3 ? 2.f / 3.f : 1.f); }

// ------------------------------------------------------------------ filter
static constexpr int kNumStyles = 6;
struct StyleDef { float drive; int nl; float kMin; float bassLoss; };
//  nl: 0 linear, 1 soft-limited state, 2 tanh integrator, 3 hard-clipped state
inline const StyleDef& styleDef(int s) {
    static const StyleDef t[kNumStyles] = {
        { 1.0f, 0,  0.35f, 0.0f },   // Smooth   : gentle, wide resonance
        { 1.6f, 0,  0.06f, 0.0f },   // Warm     : saturated input, linear filter
        { 1.2f, 1, -0.04f, 0.0f },   // Monolith : soft-limited, self-oscillating
        { 2.0f, 2, -0.05f, 0.0f },   // Hot      : saturating integrators, screaming peak
        { 1.8f, 1, -0.06f, 0.5f },   // Acid     : resonance thins the bass
        { 3.0f, 3, -0.08f, 0.0f },   // Raw      : hard-clipped feedback, buzzy self-oscillation
    };
    return t[s < 0 ? 0 : (s >= kNumStyles ? kNumStyles - 1 : s)];
}
inline const float* bwK(int stages) {
    static const float k1[1] = { 1.41421356f };
    static const float k2[2] = { 1.84775907f, 0.76536686f };
    static const float k4[4] = { 1.96157056f, 1.66293922f, 1.11114047f, 0.39018064f };
    return stages == 1 ? k1 : (stages == 2 ? k2 : k4);
}
struct FilterChan {
    float ic1[4] = {}, ic2[4] = {};
    void reset() { for (int i = 0; i < 4; ++i) ic1[i] = ic2[i] = 0; }
    // r = resonance shaped as reso^0.6. Unity small-signal gain so it can sit inside a feedback loop.
    inline float process(float x, float g, float r, int type, int stages, int style) {
        const StyleDef& sd = styleDef(style);
        if (sd.drive != 1.f) {
            if (sd.nl == 3) x = (x * sd.drive) / (1.f + std::fabs(x * sd.drive)) / sd.drive;
            else            x = std::tanh(x * sd.drive) / sd.drive;
        }
        if (sd.bassLoss > 0.f) x *= 1.f - sd.bassLoss * r;
        const float* kb = bwK(stages);
        for (int i = 0; i < stages; ++i) {
            const float k = i == stages - 1 ? kb[i] * (1.f - r) + sd.kMin * r : kb[i];
            const float a1 = 1.f / (1.f + g * (g + k));
            const float v3 = x - ic2[i];
            const float v1 = a1 * ic1[i] + g * a1 * v3;
            const float v2 = ic2[i] + g * v1;
            switch (sd.nl) {
            case 1: ic1[i] = 2.f * v1 - ic1[i]; ic1[i] = ic1[i] / std::sqrt(1.f + 0.08f * ic1[i] * ic1[i]); break;
            case 2: ic1[i] = 2.f * 2.5f * std::tanh(v1 / 2.5f) - ic1[i]; break;
            case 3: ic1[i] = clampf(2.f * v1 - ic1[i], -1.f, 1.f); break;
            default: ic1[i] = 2.f * v1 - ic1[i]; break;
            }
            ic2[i] = 2.f * v2 - ic2[i];
            x = type == 0 ? v2 : (type == 2 ? x - k * v1 - v2 : v1 * std::max(std::fabs(k), 0.12f));
        }
        return x;
    }
};

// ------------------------------------------------------------------ envelope (audio- or MIDI-triggered)
struct ADSR {
    enum Stage { Idle, Attack, Decay, Sustain, Release } stage = Idle;
    float value = 0.f, s = 0.5f, coefA = 0, coefD = 0, coefR = 0;
    static float coef(float t, float sr) { return 1.f - std::exp(-1.f / std::max(t * sr, 1.f)); }
    // sr here is the rate at which process() is called
    void set(float sr, float A, float D, float S, float R) { s = clampf(S, 0.f, 1.f); coefA = coef(A / 1.5f, sr); coefD = coef(D / 4.f, sr); coefR = coef(R / 4.f, sr); }
    void gate(bool on) { stage = on ? Attack : (stage == Idle ? Idle : Release); }
    bool gated() const { return stage == Attack || stage == Decay || stage == Sustain; }
    float process() {
        switch (stage) {
        case Attack:  value += (1.2f - value) * coefA; if (value >= 1.f) { value = 1.f; stage = Decay; } break;
        case Decay:   value += (s - value) * coefD; if (std::fabs(value - s) < 1e-4f) { value = s; stage = Sustain; } break;
        case Sustain: value = s; break;
        case Release: value -= value * coefR; if (value < 1e-5f) { value = 0.f; stage = Idle; } break;
        case Idle:    value = 0.f; break;
        }
        return value;
    }
};

// ------------------------------------------------------------------ LFO
struct Lfo {
    double phase = 0; float rndA = 0, rndB = 0, held = 0; uint32_t rng = 1;
    float rnd() { rng = rng * 1664525u + 1013904223u; return (int32_t)rng * (1.f / 2147483648.f); }
    static float warp(float t, float bal) {
        bal = clampf(bal, 0.02f, 0.98f);
        return t < bal ? 0.5f * t / bal : 0.5f + 0.5f * (t - bal) / (1.f - bal);
    }
    // wave: 0 triangle, 1 sine, 2 square, 3 random (gliding), 4 sample & hold
    float value(int wave, float bal) const {
        const float t = (float)phase;
        switch (wave) {
        case 1: return -std::cos(2.f * kPi * warp(t, bal));
        case 2: return t < clampf(bal, 0.02f, 0.98f) ? 1.f : -1.f;
        case 3: return rndA + (rndB - rndA) * t;
        case 4: return held;
        default: { const float u = warp(t, bal); return u < 0.5f ? -1.f + 4.f * u : 3.f - 4.f * u; }
        }
    }
    void wrapped() { rndA = rndB; rndB = rnd(); held = rnd(); }
    void advance(double inc) { phase += inc; if (phase >= 1.0) { phase -= std::floor(phase); wrapped(); } }
};

// ------------------------------------------------------------------ delay line
struct DelayLine {
    std::vector<float> buf; uint32_t mask = 0, w = 0;
    void alloc(uint32_t minSize) { uint32_t n = 1; while (n < minSize) n <<= 1; buf.assign(n, 0.f); mask = n - 1; w = 0; }
    void clear() { std::fill(buf.begin(), buf.end(), 0.f); w = 0; }
    void write(float x) { buf[w] = x; w = (w + 1) & mask; }
    // read the signal from d samples ago (d >= 2; called before this sample is written), cubic Hermite interpolation
    float read(float d) const {
        const float pos = (float)w - d + (float)(mask + 1);
        const int i = (int)pos; const float t = pos - (float)i;
        const float xm1 = buf[(i - 1) & mask], x0 = buf[i & mask], x1 = buf[(i + 1) & mask], x2 = buf[(i + 2) & mask];
        const float c1 = 0.5f * (x1 - xm1), c2 = xm1 - 2.5f * x0 + 2.f * x1 - 0.5f * x2, c3 = 0.5f * (x2 - xm1) + 1.5f * (x0 - x1);
        return ((c3 * t + c2) * t + c1) * t + x0;
    }
};

// one read head per channel: "tape" slews a single head, "stretch" crossfades between two heads
struct Head {
    float cur = 1000, next = 1000, fade = 1;   // delays in samples; fade 1 = settled on cur
    bool fading = false;
};

// ------------------------------------------------------------------ engine
class Engine {
public:
    float p[kParamCount];

    Engine() {
        std::fill(p, p + kParamCount, 0.f);
        p[kTimeL] = 375; p[kTimeR] = 500; p[kSyncL] = 2; p[kSyncR] = 1; p[kDivL] = 3; p[kDivR] = 4;
        p[kOffL] = 0; p[kOffR] = 0; p[kLink] = 0; p[kMode] = 0; p[kGlide] = 0.15f;
        p[kPanL] = -0.6f; p[kPanR] = 0.6f; p[kFbL] = 0.4f; p[kFbR] = 0.4f; p[kXfLR] = 0; p[kXfRL] = 0;
        p[kDrive] = 0.1f;
        p[kF1On] = 1; p[kF1Style] = 0; p[kF1Type] = 0; p[kF1Slope] = 0; p[kF1Freq] = 6000; p[kF1Peak] = 0.1f;
        p[kF2On] = 1; p[kF2Style] = 0; p[kF2Type] = 2; p[kF2Slope] = 0; p[kF2Freq] = 150;  p[kF2Peak] = 0.1f;
        p[kRouting] = 0;
        p[kInGain] = 0; p[kMix] = 0.35f; p[kOutGain] = 0;
        p[kL1Wave] = 1; p[kL1Balance] = 0.5f; p[kL1Rate] = 0.5f; p[kL1Div] = 4;
        p[kL2Wave] = 3; p[kL2Balance] = 0.5f; p[kL2Rate] = 2.f;  p[kL2Div] = 6;
        p[kEgAttack] = 0.01f; p[kEgDecay] = 0.3f; p[kEgSustain] = 0.5f; p[kEgRelease] = 0.4f; p[kEgThreshold] = -30; p[kEgTrigger] = 0;
        p[kMidiOn] = 1;
        for (int s = 0; s < kNumSlots; ++s) p[slotParam(s, kSlotAmt)] = 0.f;
    }
    void setSlot(int s, int src, int dst, float amt) {
        p[slotParam(s, kSlotSrc)] = (float)src; p[slotParam(s, kSlotDst)] = (float)dst;
        p[slotParam(s, kSlotAmt)] = amt; p[slotParam(s, kSlotBypass)] = 0;
    }

    void setSampleRate(float sr) {
        fs = sr;
        const uint32_t need = (uint32_t)(kMaxDelayMs * 0.001f * sr * 1.02f) + 16;
        for (DelayLine& d : line) if (d.buf.size() < need || d.buf.size() > 4 * need) d.alloc(need);
        smooth = 1.f - std::exp(-1.f / (0.010f * sr));
        modSmooth = 1.f - std::exp(-1.f / (0.002f * sr));
        followAtk = 1.f - std::exp(-1.f / (0.001f * sr));
        followRel = 1.f - std::exp(-1.f / (0.050f * sr));
        reset();
    }
    void reset() {
        for (DelayLine& d : line) d.clear();
        for (auto& f : flt) for (auto& c : f) c.reset();
        uint32_t seed = 4242;
        for (Lfo& l : lfo) { l.phase = 0; seed = seed * 1664525u + 1013904223u; l.rng = seed | 1; l.rndB = l.rnd(); }
        eg.stage = ADSR::Idle; eg.value = 0; level = 0; held = 0; lastNote = -1; vel = 1; chanAT = 0; polyAT = 0; bend = 0; wheel = 0;
        dcL = dcR = dcXl = dcXr = 0; ctl = 0;
        std::fill(d, d + kNumDests, 0.f);
        computeTargets();
        for (int c = 0; c < 2; ++c) { head[c].cur = head[c].next = tgtDelay[c]; head[c].fade = 1; head[c].fading = false; sTape[c] = tgtDelay[c]; sMul[c] = 1; }
        snapSmoothing();
    }
    void setTransport(bool valid, bool playing, double barPos, double bpm, double bpb) {
        tValid = valid; tPlaying = playing; tBarPos = barPos; tBpm = bpm > 1 ? bpm : 120; tBpb = bpb > 0 ? bpb : 4;
    }

    // ---------------------------------------------------------------- MIDI
    void noteOn(int note, int v) {
        if (p[kMidiOn] < 0.5f) return;
        if (held < 16) stack[held++] = note;
        lastNote = note; vel = v / 127.f; polyAT = 0;
        for (int l = 0; l < 2; ++l) if (p[kL1NoteSync + 6 * l] > 0.5f) lfo[l].phase = 0;
        if (p[kEgTrigger] > 0.5f) eg.gate(true);
    }
    void noteOff(int note) {
        if (p[kMidiOn] < 0.5f) return;
        int j = 0; for (int i = 0; i < held; ++i) if (stack[i] != note) stack[j++] = stack[i];
        held = j;
        if (held > 0) lastNote = stack[held - 1];
        else if (p[kEgTrigger] > 0.5f) eg.gate(false);
    }
    void allNotesOff() { held = 0; if (p[kEgTrigger] > 0.5f) eg.gate(false); }
    void pitchBend(int v14) { bend = (v14 - 8192) / 8192.f; }
    void modWheel(int v) { wheel = v / 127.f; }
    void channelPressure(int v) { chanAT = v / 127.f; }
    void polyPressure(int note, int v) { if (note == lastNote) polyAT = v / 127.f; }
    void clearDelays() { for (DelayLine& dl : line) dl.clear(); for (auto& f : flt) for (auto& c : f) c.reset(); }

    // ---------------------------------------------------------------- read-back for the UI
    float liveTimeMs(int c) const { return (head[c].fading ? head[c].next : (p[kMode] > 0.5f ? head[c].cur : sTape[c] * sMul[c])) * 1000.f / fs; }
    float targetTimeMs(int c) const { return baseMs[c]; }
    float egValue() const { return eg.value; }
    float inputLevel() const { return level; }
    float bpm() const { return (float)tBpm; }

    // delay time (ms) a side would have from its own controls, before modulation
    float sideMs(int c) const {
        const int s = (p[kLink] > 0.5f) ? 0 : c;
        const int sync = (int)(p[kSyncL + s] + 0.5f);
        float ms;
        if (sync == 0) ms = p[kTimeL + s];
        else ms = noteWhole((int)(p[kDivL + s] + 0.5f)) * 4.f * syncFactor(sync) * 60000.f / (float)tBpm * (1.f + p[kOffL + s] * 0.01f);
        return clampf(ms, kMinDelayMs, kMaxDelayMs);
    }

    // ---------------------------------------------------------------- render
    void process(const float* inL, const float* inR, float* outL, float* outR, uint32_t n) {
        // active slots, collected once per block
        nActive = 0;
        for (int s = 0; s < kNumSlots; ++s) {
            const int src = (int)(p[slotParam(s, kSlotSrc)] + 0.5f), dst = (int)(p[slotParam(s, kSlotDst)] + 0.5f);
            if (src <= kSrcNone || src >= kNumSources || dst <= kDstNone || dst >= kNumDests) continue;
            if (p[slotParam(s, kSlotBypass)] > 0.5f) continue;
            act[nActive++] = { s, src, dst };
        }
        bool fOn[2]; int fType[2], fStages[2], fStyle[2];
        for (int f = 0; f < 2; ++f) {
            const uint32_t b = f == 0 ? kF1On : kF2On;
            fOn[f] = p[b] > 0.5f; fStyle[f] = (int)(p[b + 1] + 0.5f); fType[f] = (int)(p[b + 2] + 0.5f);
            const int sl = (int)(p[b + 3] + 0.5f); fStages[f] = sl == 0 ? 1 : (sl == 1 ? 2 : 4);
        }
        const bool parallel = p[kRouting] > 0.5f;
        const int inputMode = (int)(p[kInput] + 0.5f);
        const bool stretch = p[kMode] > 0.5f;
        const bool egMidi = p[kEgTrigger] > 0.5f;
        const float thr = dbToGain(p[kEgThreshold]);
        const float glide = std::max(p[kGlide], 0.001f);
        const float tapeCoef = 1.f - std::exp(-1.f / (glide * 0.33f * fs));
        const float fadeInc = 1.f / (clampf(glide, 0.01f, 1.f) * fs);
        const float invL = p[kInvL] > 0.5f ? -1.f : 1.f, invR = p[kInvR] > 0.5f ? -1.f : 1.f;

        // LFO increments; synced LFOs without note sync lock to song position while the transport runs
        for (int l = 0; l < 2; ++l) {
            const uint32_t b = kL1Wave + 6 * l;
            const int sy = (int)(p[b + 2] + 0.5f);
            if (sy == 0) lfoInc[l] = p[b + 3] / fs;
            else {
                const double bars = divBars((int)(p[b + 4] + 0.5f)) * syncFactor(sy);
                lfoInc[l] = tBpm / (60.0 * fs * tBpb) / bars;
                lfoLock[l] = tValid && tPlaying && p[b + 5] < 0.5f;
                if (lfoLock[l]) { const double ph = tBarPos / bars; const double before = lfo[l].phase; lfo[l].phase = ph - std::floor(ph); if (lfo[l].phase < before - 0.5) lfo[l].wrapped(); }
            }
            if (sy == 0) lfoLock[l] = false;
        }

        for (uint32_t i = 0; i < n; ++i) {
            const float xl = inL[i], xr = inR[i];
            const float a = std::max(std::fabs(xl), std::fabs(xr));
            level += (a - level) * (a > level ? followAtk : followRel);
            if (!egMidi) {
                if (!eg.gated() && level > thr) eg.gate(true);
                else if (eg.gated() && level < thr * 0.7f) eg.gate(false);
            }
            for (int l = 0; l < 2; ++l) lfo[l].advance(lfoInc[l] * lfoRateMul[l]);

            if (--ctl <= 0) { ctl = kCtl; controlUpdate(fOn); }

            // smoothed scalars
            for (int k = 0; k < kNumSmooth; ++k) sv[k] += (tv[k] - sv[k]) * smooth;
            for (int c = 0; c < 2; ++c) sMul[c] += (tgtMul[c] - sMul[c]) * modSmooth;

            // read heads
            float tap[2];
            for (int c = 0; c < 2; ++c) {
                Head& h = head[c];
                if (!stretch) {
                    sTape[c] += (tgtDelay[c] - sTape[c]) * tapeCoef;
                    const float dd = clampf(sTape[c] * sMul[c], 2.f, maxSamples());
                    h.cur = h.next = dd; h.fading = false; h.fade = 1;
                    tap[c] = line[c].read(dd);
                } else {
                    sTape[c] = tgtDelay[c];
                    const float want = clampf(tgtDelay[c] * tgtMul[c], 2.f, maxSamples());
                    if (!h.fading && std::fabs(want - h.cur) > std::max(0.5f, 0.002f * h.cur)) { h.next = want; h.fade = 0; h.fading = true; }
                    if (h.fading) {
                        h.fade += fadeInc;
                        if (h.fade >= 1.f) { h.cur = h.next; h.fading = false; h.fade = 1; tap[c] = line[c].read(h.cur); }
                        else {
                            const float ga = std::cos(0.5f * kPi * h.fade), gb = std::sin(0.5f * kPi * h.fade);
                            tap[c] = ga * line[c].read(h.cur) + gb * line[c].read(h.next);
                        }
                    } else tap[c] = line[c].read(h.cur);
                }
            }

            // filters in the loop
            float y[2];
            for (int c = 0; c < 2; ++c) {
                float o[2] = { tap[c], tap[c] };
                for (int f = 0; f < 2; ++f) {
                    if (!fOn[f]) continue;
                    const float in = (parallel || f == 0 || !fOn[0]) ? tap[c] : o[0];
                    o[f] = flt[f][c].process(in, fg[f][c], fpk[f], fType[f], fStages[f], fStyle[f]);
                }
                if (parallel) y[c] = (fOn[0] && fOn[1]) ? 0.7071f * (o[0] + o[1]) : (fOn[0] ? o[0] : (fOn[1] ? o[1] : tap[c]));
                else          y[c] = fOn[1] ? o[1] : (fOn[0] ? o[0] : tap[c]);
            }

            // feedback, cross-feedback and per-channel phase invert, into the (saturating) tape
            const float gin = sv[kSmIn];
            float il = xl * gin, ir = xr * gin;
            if (inputMode == 1) il = ir = 0.5f * (il + ir);                 // mono: the sum feeds both lines
            else if (inputMode == 2) { il = il + ir; ir = 0; }              // ping-pong: the sum feeds the left line only
            const float wl = il + invL * (sv[kSmFbL] * y[0] + sv[kSmXfRL] * y[1]);
            const float wr = ir + invR * (sv[kSmFbR] * y[1] + sv[kSmXfLR] * y[0]);
            line[0].write(sat(wl, sv[kSmDrive]));
            line[1].write(sat(wr, sv[kSmDrive]));

            // wet: each delay line panned, with drive make-up
            const float mk = 1.f + 2.f * sv[kSmDrive];
            const float wetL = (y[0] * sv[kSmPLl] + y[1] * sv[kSmPRl]) * mk;
            const float wetR = (y[0] * sv[kSmPLr] + y[1] * sv[kSmPRr]) * mk;
            const float mix = sv[kSmMix];
            float ol = ((1.f - mix) * xl + mix * wetL) * sv[kSmOut];
            float orr = ((1.f - mix) * xr + mix * wetR) * sv[kSmOut];
            float dl = ol - dcXl + 0.9997f * dcL; dcXl = ol; dcL = dl;
            float dr = orr - dcXr + 0.9997f * dcR; dcXr = orr; dcR = dr;
            outL[i] = softCeil(dl); outR[i] = softCeil(dr);
        }
        // denormal / runaway guard on the filter states
        for (auto& f : flt) for (auto& c : f) for (int k = 0; k < 4; ++k) {
            if (!(std::fabs(c.ic1[k]) < 1e4f)) c.ic1[k] = 0;
            if (!(std::fabs(c.ic2[k]) < 1e4f)) c.ic2[k] = 0;
        }
    }

    // display helper
    static float lfoShape(float t, int wave, float bal) {
        if (wave >= 3) { const float steps[8] = { .2f, -.7f, .9f, -.1f, .5f, -.9f, .3f, -.4f }; const int k = (int)(t * 8) % 8;
                         return wave == 4 ? steps[k] : steps[k] + (steps[(k + 1) % 8] - steps[k]) * (t * 8 - (int)(t * 8)); }
        Lfo l; l.phase = t; return l.value(wave, bal);
    }

private:
    struct ActiveSlot { int slot, src, dst; };
    enum { kSmIn, kSmOut, kSmMix, kSmFbL, kSmFbR, kSmXfLR, kSmXfRL, kSmDrive, kSmPLl, kSmPLr, kSmPRl, kSmPRr, kNumSmooth };

    float maxSamples() const { return kMaxDelayMs * 0.001f * fs; }

    static float softCeil(float x) {
        const float a = std::fabs(x);
        if (!(a < 1e6f)) return 0.f;
        return a > 0.89f ? std::copysign(0.89f + 0.11f * std::tanh((a - 0.89f) / 0.11f), x) : x;
    }
    // tape saturation: transparent below 1 with no drive; drive blends in tanh compression
    static float sat(float x, float drive) {
        if (!(std::fabs(x) < 1e6f)) return 0.f;
        const float ax = std::fabs(x);
        const float clean = ax < 1.f ? x : std::copysign(1.f + 0.5f * std::tanh((ax - 1.f) / 0.5f), x);
        if (drive <= 0.001f) return clean;
        const float g = 1.f + 3.f * drive, m = std::min(1.f, drive * 4.f);
        return (1.f - m) * clean + m * std::tanh(g * x) / g;
    }

    void computeTargets() {
        for (int c = 0; c < 2; ++c) { baseMs[c] = sideMs(c); tgtDelay[c] = baseMs[c] * 0.001f * fs; tgtMul[c] = std::pow(2.f, 5.f * d[kDstTimeL + c]); }
    }

    void controlUpdate(const bool* fOn) {
        // sources
        float src[kNumSources];
        src[kSrcNone] = 0;
        for (int l = 0; l < 2; ++l) {
            const uint32_t b = kL1Wave + 6 * l;
            src[kSrcLfo1 + l] = lfo[l].value((int)(p[b] + 0.5f), clampf(p[b + 1] + d[kDstL1Balance + 2 * l], 0.f, 1.f));
        }
        eg.set(fs / kCtl, p[kEgAttack] * std::pow(2.f, 4.f * d[kDstEgAttack]), p[kEgDecay] * std::pow(2.f, 4.f * d[kDstEgDecay]),
               p[kEgSustain] + d[kDstEgSustain], p[kEgRelease] * std::pow(2.f, 4.f * d[kDstEgRelease]));
        src[kSrcEg] = eg.process();
        src[kSrcVelocity] = vel; src[kSrcAftertouch] = std::max(chanAT, polyAT);
        src[kSrcKey] = lastNote >= 0 ? (lastNote - 60) / 60.f : 0.f; src[kSrcBend] = bend; src[kSrcWheel] = wheel;

        // slot amounts first (slots can modulate slots), then destinations
        float amt[kNumSlots];
        for (int k = 0; k < nActive; ++k) amt[act[k].slot] = p[slotParam(act[k].slot, kSlotAmt)];
        for (int k = 0; k < nActive; ++k)
            if (act[k].dst >= kDstSlotFirst) {
                const int t = act[k].dst - kDstSlotFirst;
                amt[t] = clampf(amt[t] + p[slotParam(act[k].slot, kSlotAmt)] * src[act[k].src], -1.f, 1.f);
            }
        std::fill(d, d + kDstSlotFirst, 0.f);
        for (int k = 0; k < nActive; ++k)
            if (act[k].dst < kDstSlotFirst) d[act[k].dst] += amt[act[k].slot] * src[act[k].src];
        for (int k = 0; k < kNumSlots; ++k) liveAmt[k] = p[slotParam(k, kSlotAmt)];
        for (int k = 0; k < nActive; ++k) liveAmt[act[k].slot] = amt[act[k].slot];
        for (int l = 0; l < 2; ++l) lfoRateMul[l] = lfoLock[l] ? 1.f : std::pow(2.f, 4.f * d[kDstL1Rate + 2 * l]);

        computeTargets();

        // scalar targets
        tv[kSmIn] = dbToGain(p[kInGain] + 24.f * d[kDstInGain]);
        tv[kSmOut] = dbToGain(p[kOutGain] + 24.f * d[kDstOutGain]);
        tv[kSmMix] = clampf(p[kMix] + d[kDstMix], 0.f, 1.f);
        tv[kSmFbL] = clampf(p[kFbL] + d[kDstFbL], 0.f, 1.1f);
        tv[kSmFbR] = clampf(p[kFbR] + d[kDstFbR], 0.f, 1.1f);
        tv[kSmXfLR] = clampf(p[kXfLR] + d[kDstXfLR], 0.f, 1.1f);
        tv[kSmXfRL] = clampf(p[kXfRL] + d[kDstXfRL], 0.f, 1.1f);
        tv[kSmDrive] = clampf(p[kDrive] + d[kDstDrive], 0.f, 1.f);
        for (int c = 0; c < 2; ++c) {
            const float pan = clampf(p[kPanL + c] + d[kDstPanL + c], -1.f, 1.f);
            const float an = (pan + 1.f) * 0.25f * kPi;
            tv[c == 0 ? kSmPLl : kSmPRl] = std::cos(an) * 1.4142f;
            tv[c == 0 ? kSmPLr : kSmPRr] = std::sin(an) * 1.4142f;
        }
        // balance law: centre = unity in both outputs, hard pan = unity on one side
        for (int k = kSmPLl; k <= kSmPRr; ++k) tv[k] = std::min(tv[k], 1.f);

        // filter coefficients (cutoff smoothed in octaves)
        for (int f = 0; f < 2; ++f) {
            const uint32_t b = f == 0 ? kF1On : kF2On;
            const float tgtOct = std::log2(clampf(p[b + 4], 10.f, 22000.f)) + 5.f * d[kDstF1Freq + 3 * f];
            const float c8 = 1.f - std::pow(1.f - smooth, (float)kCtl);
            sOct[f] += (tgtOct - sOct[f]) * c8;
            sPeak[f] += (clampf(p[b + 5] + d[kDstF1Peak + 3 * f], 0.f, 1.f) - sPeak[f]) * c8;
            sFPan[f] += (clampf(p[b + 6] + d[kDstF1Pan + 3 * f], -1.5f, 1.5f) - sFPan[f]) * c8;
            liveHz[f] = std::pow(2.f, sOct[f]);
            if (!fOn[f]) continue;
            fpk[f] = std::pow(sPeak[f], 0.6f);
            fg[f][0] = std::tan(kPi * clampf(std::pow(2.f, sOct[f] - 2.f * sFPan[f]), 10.f, 0.45f * fs) / fs);
            fg[f][1] = std::tan(kPi * clampf(std::pow(2.f, sOct[f] + 2.f * sFPan[f]), 10.f, 0.45f * fs) / fs);
        }
    }
    void snapSmoothing() {
        bool on[2] = { true, true };
        controlUpdate(on);
        for (int k = 0; k < kNumSmooth; ++k) sv[k] = tv[k];
        for (int f = 0; f < 2; ++f) {
            const uint32_t b = f == 0 ? kF1On : kF2On;
            sOct[f] = std::log2(clampf(p[b + 4], 10.f, 22000.f)); sPeak[f] = p[b + 5]; sFPan[f] = p[b + 6];
        }
        controlUpdate(on);
        for (int c = 0; c < 2; ++c) sMul[c] = tgtMul[c];
    }

public:
    float liveHz[2] = { 1000, 1000 };
    float liveAmt[kNumSlots] = {};

private:
    DelayLine line[2];
    Head head[2];
    FilterChan flt[2][2];
    Lfo lfo[2];
    ADSR eg;
    ActiveSlot act[kNumSlots]; int nActive = 0;
    float d[kNumDests] = {};
    float tv[kNumSmooth] = {}, sv[kNumSmooth] = {};
    float tgtDelay[2] = { 1000, 1000 }, tgtMul[2] = { 1, 1 }, sTape[2] = { 1000, 1000 }, sMul[2] = { 1, 1 }, baseMs[2] = { 375, 500 };
    float sOct[2] = { 10, 10 }, sPeak[2] = {}, sFPan[2] = {}, fpk[2] = {}, fg[2][2] = { { 0.1f, 0.1f }, { 0.1f, 0.1f } };
    float lfoRateMul[2] = { 1, 1 };
    double lfoInc[2] = { 0, 0 }; bool lfoLock[2] = { false, false };
    float fs = 48000.f, smooth = 0.01f, modSmooth = 0.1f, followAtk = 0.1f, followRel = 0.01f;
    float level = 0, vel = 1, chanAT = 0, polyAT = 0, bend = 0, wheel = 0;
    float dcL = 0, dcR = 0, dcXl = 0, dcXr = 0;
    int ctl = 0;
    int stack[16] = {}; int held = 0, lastNote = -1;
    bool tValid = false, tPlaying = false; double tBarPos = 0, tBpm = 120, tBpb = 4;
};

} // namespace strata2
