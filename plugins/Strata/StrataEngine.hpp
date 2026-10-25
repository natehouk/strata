// Strata — stereo tape-style delay engine: up to 16 taps, up to six filters in the feedback loop (pass, bell,
// shelf, notch), five loop effects (drive, lo-fi, diffuse, dynamics, pitch), freeze, left/right or mid/side
// processing, the 2.x 24-slot modulation ("classic") and the shared modulation system v2 (50 slots).
// Clean-room implementation from public, generic techniques: circular delay lines with cubic Hermite
// interpolation, a slewed read head ("tape": time changes bend pitch) or crossfaded read heads
// ("stretch": time changes without pitch change), cascaded TPT state-variable filters (Zavalishin)
// with per-style nonlinearities, Schroeder allpass diffusion, sample-and-hold/bit reduction, a
// feed-forward compressor/expander, and a two-grain delay-line pitch shifter.
// No third-party product code, assets or disassembly were used.
// License: MIT (see LICENSE).

#pragma once
#include "common/ModSystem.hpp"
#include <cmath>
#include <cstdint>
#include <algorithm>
#include <vector>

namespace strata {

static constexpr float kPi = 3.14159265358979f;
inline float clampf(float x, float lo, float hi) { return x < lo ? lo : (x > hi ? hi : x); }
inline float dbToGain(float db) { return std::pow(10.f, db / 20.f); }

namespace em = epsilon::mod;
using em::kXlfoFields; using em::kEgFields; using em::kEfFields; using em::kSlotFields; using em::kNumSteps;

static constexpr int kNumSlots = 24;                    // classic (2.x) slots
static constexpr int kNumSlots2 = 50;                   // modulation system v2 slots
static constexpr int kNumFilters = 6, kNumTaps = 15;
static constexpr float kMinDelayMs = 1.f, kMaxDelayMs = 4000.f;
static constexpr int kCtl = 8;                          // modulation is evaluated every kCtl samples

// ------------------------------------------------------------------ params (append-only)
#define FLT(n)  kF##n##On, kF##n##Style, kF##n##Type, kF##n##Slope, kF##n##Freq, kF##n##Peak, kF##n##Pan
#define LFOP(n) kL##n##Wave, kL##n##Balance, kL##n##Sync, kL##n##Rate, kL##n##Div, kL##n##NoteSync
enum FField { kFOn, kFStyle, kFType, kFSlope, kFFreq, kFPeak, kFPan, kFBaseFields, kFGain = kFBaseFields, kFAllFields };
enum TapField { kTOn, kTTime, kTLevel, kTPan, kTapFields };
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
    // ---- 3.0
    kChanMode,                                  // Left/Right, Mid/Side
    kFreeze, kWidth, kWetLevel, kWetPan, kLockMix,
    kLofiOn, kLofi, kDiffOn, kDiffuse, kDynOn, kDyn, kPitchOn, kPitch, kPitchMirror, kPitchInLoop,
    kMainOn, kMainLevel, kMainPan,
    kTapFirst,                                  // 15 extra taps: on, time (fraction of the delay), level, pan
    kFGainFirst = kTapFirst + kNumTaps * kTapFields,           // filters 1-6: EQ gain
    kF36First = kFGainFirst + kNumFilters,                     // filters 3-6: the fields filters 1 and 2 have
    kXlfoFirst = kF36First + 4 * kFBaseFields,
    kEgFirst = kXlfoFirst + em::kNumXlfo * kXlfoFields,
    kEfFirst = kEgFirst + em::kNumEg * kEgFields,
    kXyFirst = kEfFirst + em::kNumEf * kEfFields,
    kSlot2First = kXyFirst + em::kNumXy * 2,
    kParamCount = kSlot2First + kNumSlots2 * kSlotFields
};
#undef FLT
#undef LFOP
enum SlotField { kSlotSrc = 0, kSlotDst = 1, kSlotAmt = 2, kSlotBypass = 3 };
constexpr uint32_t slotParam(int slot, int field) { return kSlotFirst + slot * 4 + field; }
constexpr uint32_t fpar(int f, int field) {
    return field == kFGain ? kFGainFirst + (uint32_t)f : (f == 0 ? kF1On + field : (f == 1 ? kF2On + field : kF36First + (uint32_t)((f - 2) * kFBaseFields + field)));
}
constexpr uint32_t tpar(int t, int field) { return kTapFirst + (uint32_t)(t * kTapFields + field); }
constexpr uint32_t lp_(int l, int f) { return kXlfoFirst + l * kXlfoFields + f; }
constexpr uint32_t ep(int e, int f) { return kEgFirst + e * kEgFields + f; }
constexpr uint32_t fp(int e, int f) { return kEfFirst + e * kEfFields + f; }
constexpr uint32_t xyp(int x, int axis) { return kXyFirst + x * 2 + axis; }
constexpr uint32_t sp(int s, int f) { return kSlot2First + s * kSlotFields + f; }

// classic (2.x) sources and destinations
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
// v2 destinations: 1..26 are the classic destinations (same units: amount 1 = a classic amount of 1), then the 3.0 ones
enum Dest2 {
    kV2Freeze = kDstSlotFirst, kV2Lofi, kV2Diffuse, kV2Dyn, kV2Pitch, kV2Width, kV2WetLevel, kV2WetPan,
    kV2GainFirst,                                        // filters 1-6 gain
    kV2F36First = kV2GainFirst + kNumFilters,            // filters 3-6: freq, peak, pan
    kV2TapFirst = kV2F36First + 4 * 3,                   // taps 1-15: level, time, pan
    kV2MainLevel = kV2TapFirst + kNumTaps * 3, kV2MainPan,
    kV2XlfoRateFirst, kV2XlfoBalFirst = kV2XlfoRateFirst + em::kNumXlfo,
    kV2EgFirst = kV2XlfoBalFirst + em::kNumXlfo,         // A, D, S, R per EG
    kV2SlotFirst = kV2EgFirst + em::kNumEg * 4,
    kNumDests2 = kV2SlotFirst + kNumSlots2
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
static constexpr int kNumStyles = 11, kNumStyles2 = 6;
enum Shape { kLowPass, kBandPass, kHighPass, kBell, kLowShelf, kHighShelf, kNotch, kNumShapes };
enum Slope { kSlope12, kSlope24, kSlope48, kSlope6, kNumSlopes };
enum Routing { kSerial, kParallel, kPerChannel, kNumRoutings };
struct StyleDef { float drive; int nl; float kMin; float bassLoss; int in; };
//  nl: 0 linear, 1 soft-limited state, 2 tanh integrator, 3 hard-clipped state; in: 0 as 2.x, 1 asymmetric tube, 2 hard clip
inline const StyleDef& styleDef(int s) {
    static const StyleDef t[kNumStyles] = {
        { 1.0f, 0,  0.35f, 0.0f, 0 },   // Smooth   : gentle, wide resonance
        { 1.6f, 0,  0.06f, 0.0f, 0 },   // Warm     : saturated input, linear filter
        { 1.2f, 1, -0.04f, 0.0f, 0 },   // Monolith : soft-limited, self-oscillating
        { 2.0f, 2, -0.05f, 0.0f, 0 },   // Hot      : saturating integrators, screaming peak
        { 1.8f, 1, -0.06f, 0.5f, 0 },   // Acid     : resonance thins the bass
        { 3.0f, 3, -0.08f, 0.0f, 0 },   // Raw      : hard-clipped feedback, buzzy self-oscillation
        // 3.0
        { 1.0f, 0,  0.08f, 0.0f, 0 },   // Clean    : linear, never self-oscillates
        { 1.5f, 2, -0.02f, 0.0f, 1 },   // Tube     : asymmetric, warm
        { 1.15f, 0, 0.18f, 0.0f, 0 },   // Gentle   : polite, broad
        { 2.6f, 3, -0.10f, 0.0f, 2 },   // Metal    : hard-clipped input and feedback
        { 1.3f, 1, -0.06f, 0.0f, 0 },   // Hollow   : soft-limited, scooped (see scoop below)
    };
    return t[s < 0 ? 0 : (s >= kNumStyles ? kNumStyles - 1 : s)];
}
inline const char* styleName(int s) {
    static const char* const n[kNumStyles] = { "Smooth", "Warm", "Monolith", "Hot", "Acid", "Raw", "Clean", "Tube", "Gentle", "Metal", "Hollow" };
    return n[s < 0 ? 0 : (s >= kNumStyles ? kNumStyles - 1 : s)];
}
inline const float* bwK(int stages) {
    static const float k1[1] = { 1.41421356f };
    static const float k2[2] = { 1.84775907f, 0.76536686f };
    static const float k4[4] = { 1.96157056f, 1.66293922f, 1.11114047f, 0.39018064f };
    return stages == 1 ? k1 : (stages == 2 ? k2 : k4);
}
inline int slopeStages(int slope) { return slope == kSlope12 ? 1 : (slope == kSlope24 ? 2 : (slope == kSlope48 ? 4 : 0)); }
inline float eqDamping(float peak) { return 1.41421356f * (1.f - 0.85f * std::pow(clampf(peak, 0.f, 1.f), 0.8f)); }

struct FilterChan {
    float ic1[4] = {}, ic2[4] = {}, z1 = 0, dcx = 0, dcy = 0;
    void reset() { for (int i = 0; i < 4; ++i) ic1[i] = ic2[i] = 0; z1 = dcx = dcy = 0; }
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
    // 3.0: every style and shape. Pass shapes saturate per the style; bell, shelf and notch are clean EQ shapes.
    inline float process3(float x, float g, float r, float peak, int type, int stages, int style, float gain) {
        const StyleDef& sd = styleDef(style);
        if (type <= kHighPass && stages > 0 && style < kNumStyles2) return process(x, g, r, type, stages, style);
        if (type <= kHighPass) {
            if (sd.drive != 1.f) {
                if (sd.in == 1) { const float b = 0.25f, y = (std::tanh(sd.drive * x + b) - std::tanh(b)) / sd.drive; const float o = y - dcx + 0.9995f * dcy; dcx = y; dcy = o; x = o; }
                else if (sd.in == 2) x = clampf(x * sd.drive, -1.f, 1.f) / sd.drive;
                else if (sd.nl == 3) x = (x * sd.drive) / (1.f + std::fabs(x * sd.drive)) / sd.drive;
                else x = std::tanh(x * sd.drive) / sd.drive;
            }
            if (stages == 0 && type != kBandPass) {             // 6 dB/oct one-pole
                const float G = g / (1.f + g), v = (x - z1) * G, lo = v + z1; z1 = lo + v;
                return type == kLowPass ? lo : x - lo;
            }
            if (stages == 0) stages = 1;
            const float* kb = bwK(stages);
            const bool hollow = style == 10;
            for (int i = 0; i < stages; ++i) {
                const bool last = i == stages - 1;
                const float k = last ? kb[i] * (1.f - r) + sd.kMin * r : kb[i];
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
                float y = type == kLowPass ? v2 : (type == kHighPass ? x - k * v1 - v2 : v1 * std::max(std::fabs(k), 0.12f));
                if (last && hollow) y -= 0.6f * r * v1 * std::max(std::fabs(k), 0.12f);
                x = y;
            }
            return x;
        }
        // clean EQ shapes
        if (stages == 0 && (type == kLowShelf || type == kHighShelf)) {
            const float G = g / (1.f + g), v = (x - z1) * G, lo = v + z1; z1 = lo + v;
            return type == kLowShelf ? x + (gain - 1.f) * lo : x + (gain - 1.f) * (x - lo);
        }
        const bool wide = stages == 0; if (stages == 0) stages = 1;
        const float k = std::max(eqDamping(peak) * (wide ? 2.f : 1.f), 0.02f), gs = stages == 1 ? gain : std::pow(gain, 1.f / stages);
        for (int i = 0; i < stages; ++i) {
            const float a1 = 1.f / (1.f + g * (g + k));
            const float v3 = x - ic2[i];
            const float v1 = a1 * ic1[i] + g * a1 * v3;
            const float v2 = ic2[i] + g * v1;
            ic1[i] = 2.f * v1 - ic1[i]; ic2[i] = 2.f * v2 - ic2[i];
            switch (type) {
            case kBell: x = x + (gs - 1.f) * k * v1; break;
            case kLowShelf: x = x + (gs - 1.f) * v2; break;
            case kHighShelf: x = x + (gs - 1.f) * (x - k * v1 - v2); break;
            default: x = x - k * v1; break;              // notch
            }
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

// ------------------------------------------------------------------ 3.0 loop effects
// Schroeder allpass diffusion: four allpasses, blended in by the amount
struct Diffuser {
    std::vector<float> b[4]; int pos[4] = {}, len[4] = {};
    void init(float fs, int ch) { static const float ms[4] = { 4.7f, 7.1f, 11.3f, 15.9f };
        for (int i = 0; i < 4; ++i) { len[i] = std::max(4, (int)(ms[i] * (1.f + 0.031f * ch) * 0.001f * fs)); b[i].assign(len[i], 0.f); pos[i] = 0; } }
    void clear() { for (int i = 0; i < 4; ++i) { std::fill(b[i].begin(), b[i].end(), 0.f); pos[i] = 0; } }
    inline float process(float x, float amt) {
        float y = x; const float g = 0.62f;
        for (int i = 0; i < 4; ++i) { const float d = b[i][pos[i]]; const float v = y + g * d; b[i][pos[i]] = v; y = d - g * v; if (++pos[i] >= len[i]) pos[i] = 0; }
        return x + (y - x) * amt;
    }
};
// sample-rate and bit-depth reduction
struct LoFi {
    float held = 0, cnt = 0;
    inline float process(float x, float amt) {
        const float hold = 1.f + amt * amt * 31.f;
        if ((cnt += 1.f) >= hold) { cnt -= hold; const float bits = 16.f - 12.f * amt, q = std::pow(2.f, bits - 1.f); held = std::round(x * q) / q; }
        return held;
    }
};
// two-grain delay-line pitch shifter (about 40 ms grains)
struct PitchShift {
    std::vector<float> buf; int w = 0; double ph = 0; int n = 2048;
    void init(float fs) { n = std::max(256, (int)(0.04f * fs)); int s = 1; while (s < 2 * n + 8) s <<= 1; buf.assign(s, 0.f); w = 0; ph = 0; }
    void clear() { std::fill(buf.begin(), buf.end(), 0.f); w = 0; ph = 0; }
    inline float tapAt(float d) const { const int m = (int)buf.size() - 1; const float pos = (float)w - d + (float)(m + 1); const int i = (int)pos; const float t = pos - i; return buf[i & m] + (buf[(i + 1) & m] - buf[i & m]) * t; }
    inline float process(float x, float ratio) {
        const int m = (int)buf.size() - 1; buf[w] = x; w = (w + 1) & m;
        ph += (1.0 - ratio) / n; ph -= std::floor(ph);
        const double p2 = ph + 0.5 - std::floor(ph + 0.5);
        const float s1 = std::sin(kPi * (float)ph), s2 = std::sin(kPi * (float)p2);
        return tapAt(2.f + (float)ph * n) * s1 * s1 + tapAt(2.f + (float)p2 * n) * s2 * s2;
    }
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
        // 3.0 defaults: everything new is neutral
        p[kWidth] = 1; p[kWetLevel] = 0; p[kMainOn] = 1; p[kLofi] = 0.3f; p[kDiffuse] = 0.4f; p[kDyn] = 0.3f; p[kPitch] = 12; p[kPitchInLoop] = 1;
        static const float ft[4] = { 1000, 3000, 400, 8000 };
        for (int f = 2; f < kNumFilters; ++f) { p[fpar(f, kFType)] = kBell; p[fpar(f, kFFreq)] = ft[f - 2]; p[fpar(f, kFPeak)] = 0.3f; }
        for (int t = 0; t < kNumTaps; ++t) { p[tpar(t, kTTime)] = (t + 1) / 16.f; p[tpar(t, kTLevel)] = -6; }
        for (int l = 0; l < em::kNumXlfo; ++l) for (int f = 0; f < kXlfoFields; ++f) p[lp_(l, f)] = f < em::kLStep1 ? em::xlfoRange(f).def : ((f - em::kLStep1) % 4) / 1.5f - 1.f;
        for (int e = 0; e < em::kNumEg; ++e) for (int f = 0; f < kEgFields; ++f) p[ep(e, f)] = em::egRange(f).def;
        for (int e = 0; e < em::kNumEf; ++e) for (int f = 0; f < kEfFields; ++f) p[fp(e, f)] = em::efRange(f).def;
        for (int x = 0; x < em::kNumXy * 2; ++x) p[kXyFirst + x] = 0.5f;
    }
    void setSlot(int s, int src, int dst, float amt) {
        p[slotParam(s, kSlotSrc)] = (float)src; p[slotParam(s, kSlotDst)] = (float)dst;
        p[slotParam(s, kSlotAmt)] = amt; p[slotParam(s, kSlotBypass)] = 0;
    }
    void setSlot2(int s, int src, int dst, float amt) { p[sp(s, kSlotSrc)] = (float)src; p[sp(s, kSlotDst)] = (float)dst; p[sp(s, kSlotAmt)] = amt; p[sp(s, kSlotBypass)] = 0; }
    static const em::Layout& layout() { static const em::Layout L = { kXlfoFirst, kEgFirst, kEfFirst, kXyFirst, kSlot2First, kNumSlots2, kV2SlotFirst, kNumDests2 }; return L; }

    void setSampleRate(float sr) {
        fs = sr;
        const uint32_t need = (uint32_t)(kMaxDelayMs * 0.001f * sr * 1.02f) + 16;
        for (DelayLine& d : line) if (d.buf.size() < need || d.buf.size() > 4 * need) d.alloc(need);
        smooth = 1.f - std::exp(-1.f / (0.010f * sr));
        modSmooth = 1.f - std::exp(-1.f / (0.002f * sr));
        followAtk = 1.f - std::exp(-1.f / (0.001f * sr));
        followRel = 1.f - std::exp(-1.f / (0.050f * sr));
        for (int c = 0; c < 2; ++c) { diff[c].init(sr, c); pitchIn[c].init(sr); pitchOut[c].init(sr); }
        dynA = 1.f - std::exp(-1.f / (0.005f * sr)); dynR = 1.f - std::exp(-1.f / (0.12f * sr));
        reset();
    }
    void reset() {
        for (DelayLine& d : line) d.clear();
        for (auto& f : flt) for (auto& c : f) c.reset();
        uint32_t seed = 4242;
        for (Lfo& l : lfo) { l.phase = 0; seed = seed * 1664525u + 1013904223u; l.rng = seed | 1; l.rndB = l.rnd(); }
        eg.stage = ADSR::Idle; eg.value = 0; level = 0; held = 0; lastNote = -1; vel = 1; chanAT = 0; polyAT = 0; bend = 0; wheel = 0;
        dcL = dcR = dcXl = dcXr = 0; ctl = 0;
        std::fill(d, d + kNumDests2, 0.f);
        std::fill(off2, off2 + kNumDests2, 0.f); anyV2 = false;
        for (int c = 0; c < 2; ++c) { diff[c].clear(); lofi[c] = LoFi(); pitchIn[c].clear(); pitchOut[c].clear(); }
        dynEnv = 0; dynGain = 1; dynDuck = 1; sFreeze = 0;
        mods.resetFollowers();
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
        mods.noteOn(p, layout(), note, v);
    }
    void noteOff(int note) {
        if (p[kMidiOn] < 0.5f) return;
        int j = 0; for (int i = 0; i < held; ++i) if (stack[i] != note) stack[j++] = stack[i];
        held = j;
        if (held > 0) lastNote = stack[held - 1];
        else if (p[kEgTrigger] > 0.5f) eg.gate(false);
        mods.noteOff(p, layout(), note);
    }
    void allNotesOff() { held = 0; if (p[kEgTrigger] > 0.5f) eg.gate(false); mods.allNotesOff(p, layout()); }
    void pitchBend(int v14) { bend = (v14 - 8192) / 8192.f; mods.pitchBend(v14); }
    void modWheel(int v) { wheel = v / 127.f; mods.modWheel(v); }
    void channelPressure(int v) { chanAT = v / 127.f; mods.channelPressure(v); }
    void polyPressure(int note, int v) { if (note == lastNote) polyAT = v / 127.f; mods.polyPressure(note, v); }
    void clearDelays() {
        for (DelayLine& dl : line) dl.clear();
        for (auto& f : flt) for (auto& c : f) c.reset();
        for (int c = 0; c < 2; ++c) { diff[c].clear(); pitchIn[c].clear(); pitchOut[c].clear(); }
    }

    // ---------------------------------------------------------------- read-back for the UI
    float liveTimeMs(int c) const { return (head[c].fading ? head[c].next : (p[kMode] > 0.5f ? head[c].cur : sTape[c] * sMul[c])) * 1000.f / fs; }
    float targetTimeMs(int c) const { return baseMs[c]; }
    float egValue() const { return eg.value; }
    float inputLevel() const { return level; }
    float bpm() const { return (float)tBpm; }
    double xlfoPhase(int l) const { return mods.xlfo[l].phase; }
    bool frozen() const { return sFreeze > 0.5f; }

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
    void process(const float* inL, const float* inR, float* outL, float* outR, uint32_t n) { process(inL, inR, nullptr, nullptr, outL, outR, n); }
    void process(const float* inL, const float* inR, const float* scL, const float* scR, float* outL, float* outR, uint32_t n) {
        // active slots, collected once per block
        nActive = 0;
        for (int s = 0; s < kNumSlots; ++s) {
            const int src = (int)(p[slotParam(s, kSlotSrc)] + 0.5f), dst = (int)(p[slotParam(s, kSlotDst)] + 0.5f);
            if (src <= kSrcNone || src >= kNumSources || dst <= kDstNone || dst >= kNumDests) continue;
            if (p[slotParam(s, kSlotBypass)] > 0.5f) continue;
            act[nActive++] = { s, src, dst };
        }
        bool fOn[kNumFilters]; int fType[kNumFilters], fStages[kNumFilters], fStyle[kNumFilters];
        for (int f = 0; f < kNumFilters; ++f) {
            fOn[f] = p[fpar(f, kFOn)] > 0.5f; fStyle[f] = (int)(p[fpar(f, kFStyle)] + 0.5f); fType[f] = (int)(p[fpar(f, kFType)] + 0.5f);
            const int sl = (int)(p[fpar(f, kFSlope)] + 0.5f); fStages[f] = sl == 0 ? 1 : (sl == 1 ? 2 : (sl == 2 ? 4 : 0));
        }
        const int route = (int)clampf(std::round(p[kRouting]), 0, kNumRoutings - 1);
        const bool parallel = route == kParallel;
        const int inputMode = (int)(p[kInput] + 0.5f);
        const bool stretch = p[kMode] > 0.5f;
        const bool egMidi = p[kEgTrigger] > 0.5f;
        const float thr = dbToGain(p[kEgThreshold]);
        const float glide = std::max(p[kGlide], 0.001f);
        const float tapeCoef = 1.f - std::exp(-1.f / (glide * 0.33f * fs));
        const float fadeInc = 1.f / (clampf(glide, 0.01f, 1.f) * fs);
        const float invL = p[kInvL] > 0.5f ? -1.f : 1.f, invR = p[kInvR] > 0.5f ? -1.f : 1.f;
        // 3.0 features in use (none of them: the 2.x signal path, sample for sample)
        bool newFilters = route == kPerChannel;
        for (int f = 0; f < kNumFilters; ++f) if (fOn[f] && (f >= 2 || fType[f] > kHighPass || fStages[f] == 0 || fStyle[f] >= kNumStyles2)) newFilters = true;
        bool anyTap = false; for (int t = 0; t < kNumTaps; ++t) anyTap |= p[tpar(t, kTOn)] > 0.5f;
        const bool ms = p[kChanMode] > 0.5f;
        const bool v3 = newFilters || anyTap || ms || inputMode == 3 || p[kFreeze] > 0.5f || p[kWidth] != 1.f || p[kWetLevel] != 0.f || p[kWetPan] != 0.f
                     || p[kLofiOn] > 0.5f || p[kDiffOn] > 0.5f || p[kDynOn] > 0.5f || p[kPitchOn] > 0.5f || p[kMainOn] < 0.5f || p[kMainLevel] != 0.f || p[kMainPan] != 0.f
                     || anyV2 || sFreeze > 0.f;

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
        const float pitchSemi = clampf(p[kPitch] + 12.f * off2[kV2Pitch], -12.f, 12.f), pitchRatio = std::pow(2.f, pitchSemi / 12.f);
        const bool pitchOn = p[kPitchOn] > 0.5f, pitchLoop = p[kPitchInLoop] > 0.5f, mirror = p[kPitchMirror] > 0.5f;
        const bool lofiOn = p[kLofiOn] > 0.5f, diffOn = p[kDiffOn] > 0.5f, dynOn = p[kDynOn] > 0.5f;
        const float frzTarget = (p[kFreeze] + off2[kV2Freeze]) > 0.5f ? 1.f : 0.f, frzK = 1.f - std::exp(-1.f / (0.02f * fs));

        for (uint32_t i = 0; i < n; ++i) {
            const float xl = inL[i], xr = inR[i];
            const float a = std::max(std::fabs(xl), std::fabs(xr));
            level += (a - level) * (a > level ? followAtk : followRel);
            if (!egMidi) {
                if (!eg.gated() && level > thr) eg.gate(true);
                else if (eg.gated() && level < thr * 0.7f) eg.gate(false);
            }
            for (int l = 0; l < 2; ++l) lfo[l].advance(lfoInc[l] * lfoRateMul[l]);
            mods.follow(a, (scL && scR) ? std::max(std::fabs(scL[i]), std::fabs(scR[i])) : 0.f);

            if (--ctl <= 0) { ctl = kCtl; controlUpdate(fOn); }

            // smoothed scalars
            for (int k = 0; k < kNumSmooth; ++k) sv[k] += (tv[k] - sv[k]) * smooth;
            for (int c = 0; c < 2; ++c) sMul[c] += (tgtMul[c] - sMul[c]) * modSmooth;

            // read heads
            float tap[2], curD[2];
            for (int c = 0; c < 2; ++c) {
                Head& h = head[c];
                if (!stretch) {
                    sTape[c] += (tgtDelay[c] - sTape[c]) * tapeCoef;
                    const float dd = clampf(sTape[c] * sMul[c], 2.f, maxSamples());
                    h.cur = h.next = dd; h.fading = false; h.fade = 1;
                    tap[c] = line[c].read(dd); curD[c] = dd;
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
                    curD[c] = h.fading ? h.next : h.cur;
                }
            }

            if (!v3) {
                // ---- the 2.x signal path, unchanged ----
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
                const float gin = sv[kSmIn];
                float il = xl * gin, ir = xr * gin;
                if (inputMode == 1) il = ir = 0.5f * (il + ir);
                else if (inputMode == 2) { il = il + ir; ir = 0; }
                const float wl = il + invL * (sv[kSmFbL] * y[0] + sv[kSmXfRL] * y[1]);
                const float wr = ir + invR * (sv[kSmFbR] * y[1] + sv[kSmXfLR] * y[0]);
                line[0].write(sat(wl, sv[kSmDrive]));
                line[1].write(sat(wr, sv[kSmDrive]));
                const float mk = 1.f + 2.f * sv[kSmDrive];
                const float wetL = (y[0] * sv[kSmPLl] + y[1] * sv[kSmPRl]) * mk;
                const float wetR = (y[0] * sv[kSmPLr] + y[1] * sv[kSmPRr]) * mk;
                const float mix = sv[kSmMix];
                float ol = ((1.f - mix) * xl + mix * wetL) * sv[kSmOut];
                float orr = ((1.f - mix) * xr + mix * wetR) * sv[kSmOut];
                float dl = ol - dcXl + 0.9997f * dcL; dcXl = ol; dcL = dl;
                float dr = orr - dcXr + 0.9997f * dcR; dcXr = orr; dcR = dr;
                outL[i] = softCeil(dl); outR[i] = softCeil(dr);
                continue;
            }

            // ---- 3.0 signal path ----
            sFreeze += (frzTarget - sFreeze) * frzK;
            // filters in the loop (per channel: odd filters on the left/mid, even on the right/side)
            float y[2];
            for (int c = 0; c < 2; ++c) {
                float x = tap[c], acc = 0; int cnt = 0;
                for (int f = 0; f < kNumFilters; ++f) {
                    if (!fOn[f]) continue;
                    if (route == kPerChannel && (f & 1) != c) continue;
                    const bool serialF = route != kParallel;
                    const float o = flt[f][c].process3(serialF ? x : tap[c], fg[f][c], fpk[f], sPeak[f], fType[f], fStages[f], fStyle[f], fgain[f]);
                    if (serialF) x = o; else acc += o;
                    ++cnt;
                }
                y[c] = route == kParallel ? (cnt ? acc * (cnt == 2 ? 0.7071f : 1.f / std::sqrt((float)cnt)) : tap[c]) : x;
                if (pitchOn && pitchLoop) y[c] = pitchIn[c].process(y[c], (mirror && c == 1) ? 1.f / pitchRatio : pitchRatio);
            }

            // input: gain, mid/side, width, dynamics, mono / ping-pong
            const float gin = sv[kSmIn];
            float il = xl * gin, ir = xr * gin;
            if (ms) { const float m = 0.5f * (il + ir), s = 0.5f * (il - ir); il = m; ir = s * sWidth; }
            else if (sWidth != 1.f) { const float m = 0.5f * (il + ir), s = 0.5f * (il - ir) * sWidth; il = m + s; ir = m - s; }
            // dynamics: left of centre ducks the echoes while the input plays; right of centre swells the input
            // (quiet parts up, loud parts down) before it enters the lines
            float duck = 1.f;
            if (dynOn) {
                const float lv = std::max(std::fabs(il), std::fabs(ir));
                dynEnv += (lv - dynEnv) * (lv > dynEnv ? dynA : dynR);
                const float dv = clampf(p[kDyn] + off2[kV2Dyn], -1.f, 1.f), e = std::max(dynEnv, 1e-6f);
                float g = 1.f, gw = 1.f;
                if (dv > 0) {
                    const float th = 0.063f;
                    if (e > th) g = std::pow(e / th, 1.f / (1.f + dv * 9.f) - 1.f);
                    else if (e > 1e-4f) g = std::min(16.f, std::pow(e / th, -0.75f * dv));
                } else if (dv < 0) { const float th = 0.016f; if (e > th) gw = std::max(0.03f, std::pow(e / th, 1.5f * dv)); }
                dynGain += (g - dynGain) * 0.01f; dynDuck += (gw - dynDuck) * 0.01f;
                il *= dynGain; ir *= dynGain; duck = dynDuck;
            }
            if (inputMode == 1) il = ir = 0.5f * (il + ir);
            else if (inputMode == 2) { il = il + ir; ir = 0; }
            else if (inputMode == 3) { ir = il + ir; il = 0; }
            const float fr = sFreeze, live = 1.f - fr;
            il *= live; ir *= live;
            float wl = il + invL * (sv[kSmFbL] * y[0] + sv[kSmXfRL] * y[1]);
            float wr = ir + invR * (sv[kSmFbR] * y[1] + sv[kSmXfLR] * y[0]);
            if (lofiOn) { const float la = clampf(p[kLofi] + off2[kV2Lofi], 0.f, 1.f); wl = lofi[0].process(wl, la); wr = lofi[1].process(wr, la); }
            if (diffOn) { const float da = clampf(p[kDiffuse] + off2[kV2Diffuse], 0.f, 1.f); wl = diff[0].process(wl, da); wr = diff[1].process(wr, da); }
            wl = sat(wl, sv[kSmDrive]); wr = sat(wr, sv[kSmDrive]);
            if (fr > 0.f) { wl = live * wl + fr * tap[0]; wr = live * wr + fr * tap[1]; }     // freeze: the buffer recirculates untouched
            line[0].write(wl); line[1].write(wr);

            // wet: main tap (panned per line), extra taps, wet level and pan
            const float mk = 1.f + 2.f * sv[kSmDrive];
            float yl = y[0], yr = y[1];
            if (pitchOn && !pitchLoop) { yl = pitchOut[0].process(yl, pitchRatio); yr = pitchOut[1].process(yr, mirror ? 1.f / pitchRatio : pitchRatio); }
            yl *= sMainGL; yr *= sMainGR;
            float wetL = (yl * sv[kSmPLl] + yr * sv[kSmPRl]) * mk;
            float wetR = (yl * sv[kSmPLr] + yr * sv[kSmPRr]) * mk;
            if (anyTap) for (int t = 0; t < kNumTaps; ++t) {
                if (p[tpar(t, kTOn)] < 0.5f) continue;
                const float tl = line[0].read(clampf(tapTime[t] * curD[0], 2.f, maxSamples())), tr = line[1].read(clampf(tapTime[t] * curD[1], 2.f, maxSamples()));
                wetL += tl * tapGL[t]; wetR += tr * tapGR[t];
            }
            wetL *= sWetL * duck; wetR *= sWetR * duck;
            if (ms) { const float l = wetL + wetR, r = wetL - wetR; wetL = l; wetR = r; }
            const float mix = sv[kSmMix];
            float ol = ((1.f - mix) * xl + mix * wetL) * sv[kSmOut];
            float orr = ((1.f - mix) * xr + mix * wetR) * sv[kSmOut];
            float dl = ol - dcXl + 0.9997f * dcL; dcXl = ol; dcL = dl;
            float dr = orr - dcXr + 0.9997f * dcR; dcXr = orr; dcR = dr;
            outL[i] = softCeil(dl); outR[i] = softCeil(dr);
        }
        // denormal / runaway guard on the filter states
        for (auto& f : flt) for (auto& c : f) { for (int k = 0; k < 4; ++k) {
            if (!(std::fabs(c.ic1[k]) < 1e4f)) c.ic1[k] = 0;
            if (!(std::fabs(c.ic2[k]) < 1e4f)) c.ic2[k] = 0; }
            if (!(std::fabs(c.z1) < 1e4f)) c.z1 = 0; }
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

    // v2 sources and matrix; offsets in off2 (classic destinations in classic units)
    void controlV2() {
        // effective XLFO rate/balance and EG stages for the sources
        std::copy(p + kXlfoFirst, p + kSlot2First, eff2 + kXlfoFirst);
        for (int l = 0; l < em::kNumXlfo; ++l) {
            if (off2[kV2XlfoRateFirst + l] != 0.f) eff2[lp_(l, em::kLRate)] = p[lp_(l, em::kLRate)] * std::pow(2.f, 4.f * off2[kV2XlfoRateFirst + l]);
            if (off2[kV2XlfoBalFirst + l] != 0.f) eff2[lp_(l, em::kLBalance)] = clampf(p[lp_(l, em::kLBalance)] + off2[kV2XlfoBalFirst + l], 0.f, 1.f);
        }
        for (int e = 0; e < em::kNumEg; ++e) {
            static const int f[4] = { em::kEAttack, em::kEDecay, em::kESustain, em::kERelease };
            for (int k = 0; k < 4; ++k) { const float o = off2[kV2EgFirst + e * 4 + k]; if (o == 0.f) continue;
                eff2[ep(e, f[k])] = k == 2 ? clampf(p[ep(e, f[k])] + o, 0.f, 1.f) : p[ep(e, f[k])] * std::pow(2.f, 4.f * o); }
        }
        mods.tick(p, eff2, layout(), fs, fs / kCtl, tValid, tPlaying, tBarPos, tBpm);
        std::copy(mods.val, mods.val + em::kNumSources, srcVal2);
        em::matrix(p, layout(), srcVal2, off2);
        anyV2 = false; for (int k = 1; k < kV2SlotFirst; ++k) if (off2[k] != 0.f) { anyV2 = true; break; }
    }

    void controlUpdate(const bool* fOn) {
        controlV2();
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
        for (int s = 0; s < kNumSources; ++s) srcVal[s] = src[s];

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
        if (anyV2) for (int k = 1; k < kDstSlotFirst; ++k) d[k] += off2[k];      // v2 slots on the classic destinations
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
        const float c8 = 1.f - std::pow(1.f - smooth, (float)kCtl);
        for (int f = 0; f < kNumFilters; ++f) {
            float dF, dP, dN;
            if (f < 2) { dF = d[kDstF1Freq + 3 * f]; dP = d[kDstF1Peak + 3 * f]; dN = d[kDstF1Pan + 3 * f]; }
            else { const int k = kV2F36First + (f - 2) * 3; dF = off2[k]; dP = off2[k + 1]; dN = off2[k + 2]; }
            const float tgtOct = std::log2(clampf(p[fpar(f, kFFreq)], 10.f, 22000.f)) + 5.f * dF;
            sOct[f] += (tgtOct - sOct[f]) * c8;
            sPeak[f] += (clampf(p[fpar(f, kFPeak)] + dP, 0.f, 1.f) - sPeak[f]) * c8;
            sFPan[f] += (clampf(p[fpar(f, kFPan)] + dN, -1.5f, 1.5f) - sFPan[f]) * c8;
            liveHz[f] = std::pow(2.f, sOct[f]);
            if (!fOn[f]) continue;
            fpk[f] = std::pow(sPeak[f], 0.6f);
            const float panK = p[kRouting] > 1.5f ? 0.f : 2.f;        // per-channel routing: no frequency spread
            fg[f][0] = std::tan(kPi * clampf(std::pow(2.f, sOct[f] - (f < 2 && p[kRouting] < 1.5f ? 2.f : panK) * sFPan[f]), 10.f, 0.45f * fs) / fs);
            fg[f][1] = std::tan(kPi * clampf(std::pow(2.f, sOct[f] + (f < 2 && p[kRouting] < 1.5f ? 2.f : panK) * sFPan[f]), 10.f, 0.45f * fs) / fs);
            fgain[f] += (dbToGain(clampf(p[fpar(f, kFGain)] + 24.f * off2[kV2GainFirst + f], -24.f, 24.f)) - fgain[f]) * c8;
        }

        // 3.0 scalars (smoothed at the control rate)
        const float wTarget = clampf(p[kWidth] + off2[kV2Width], 0.f, 1.f);
        sWidth += (wTarget - sWidth) * c8;
        const float wl = dbToGain(clampf(p[kWetLevel] + 24.f * off2[kV2WetLevel], -48.f, 12.f)), wp = clampf(p[kWetPan] + off2[kV2WetPan], -1.f, 1.f);
        sWetL += (wl * (wp > 0 ? 1.f - wp : 1.f) - sWetL) * c8; sWetR += (wl * (wp < 0 ? 1.f + wp : 1.f) - sWetR) * c8;
        const float ml = p[kMainOn] > 0.5f ? dbToGain(clampf(p[kMainLevel] + 24.f * off2[kV2MainLevel], -60.f, 12.f)) : 0.f, mp = clampf(p[kMainPan] + off2[kV2MainPan], -1.f, 1.f);
        sMainGL += (ml * (mp > 0 ? 1.f - mp : 1.f) - sMainGL) * c8; sMainGR += (ml * (mp < 0 ? 1.f + mp : 1.f) - sMainGR) * c8;
        for (int t = 0; t < kNumTaps; ++t) {
            const int k = kV2TapFirst + t * 3;
            const float lv = dbToGain(clampf(p[tpar(t, kTLevel)] + 24.f * off2[k], -60.f, 6.f)), pn = clampf(p[tpar(t, kTPan)] + off2[k + 2], -1.f, 1.f);
            const float tt = clampf(p[tpar(t, kTTime)] + off2[k + 1], 0.01f, 1.f);
            tapTime[t] += (tt - tapTime[t]) * c8;
            tapGL[t] += (lv * (pn > 0 ? 1.f - pn : 1.f) - tapGL[t]) * c8; tapGR[t] += (lv * (pn < 0 ? 1.f + pn : 1.f) - tapGR[t]) * c8;
        }
    }
    void snapSmoothing() {
        bool on[kNumFilters] = { true, true, true, true, true, true };
        controlUpdate(on);
        for (int k = 0; k < kNumSmooth; ++k) sv[k] = tv[k];
        for (int f = 0; f < kNumFilters; ++f) {
            sOct[f] = std::log2(clampf(p[fpar(f, kFFreq)], 10.f, 22000.f)); sPeak[f] = p[fpar(f, kFPeak)]; sFPan[f] = p[fpar(f, kFPan)];
            fgain[f] = dbToGain(p[fpar(f, kFGain)]);
        }
        sWidth = p[kWidth];
        { const float wl = dbToGain(p[kWetLevel]), wp = p[kWetPan]; sWetL = wl * (wp > 0 ? 1.f - wp : 1.f); sWetR = wl * (wp < 0 ? 1.f + wp : 1.f); }
        { const float ml = p[kMainOn] > 0.5f ? dbToGain(p[kMainLevel]) : 0.f, mp = p[kMainPan]; sMainGL = ml * (mp > 0 ? 1.f - mp : 1.f); sMainGR = ml * (mp < 0 ? 1.f + mp : 1.f); }
        for (int t = 0; t < kNumTaps; ++t) { const float lv = dbToGain(p[tpar(t, kTLevel)]), pn = p[tpar(t, kTPan)]; tapTime[t] = p[tpar(t, kTTime)]; tapGL[t] = lv * (pn > 0 ? 1.f - pn : 1.f); tapGR[t] = lv * (pn < 0 ? 1.f + pn : 1.f); }
        controlUpdate(on);
        for (int c = 0; c < 2; ++c) sMul[c] = tgtMul[c];
    }

public:
    float liveHz[kNumFilters] = { 1000, 1000, 1000, 1000, 1000, 1000 };
    float liveAmt[kNumSlots] = {};
    float srcVal[kNumSources] = {};             // classic sources
    float srcVal2[em::kNumSources] = {};        // v2 sources
    float off2[kNumDests2] = {};                // v2 offsets
    float liveD[kNumDests2] = {};               // unused placeholder for older UIs

private:
    DelayLine line[2];
    Head head[2];
    FilterChan flt[kNumFilters][2];
    Lfo lfo[2];
    ADSR eg;
    em::Sources mods;
    float eff2[kParamCount] = {};
    bool anyV2 = false;
    ActiveSlot act[kNumSlots]; int nActive = 0;
    float d[kNumDests2] = {};
    float tv[kNumSmooth] = {}, sv[kNumSmooth] = {};
    float tgtDelay[2] = { 1000, 1000 }, tgtMul[2] = { 1, 1 }, sTape[2] = { 1000, 1000 }, sMul[2] = { 1, 1 }, baseMs[2] = { 375, 500 };
    float sOct[kNumFilters] = { 10, 10, 10, 10, 10, 10 }, sPeak[kNumFilters] = {}, sFPan[kNumFilters] = {}, fpk[kNumFilters] = {};
    float fg[kNumFilters][2] = { { 0.1f, 0.1f }, { 0.1f, 0.1f }, { 0.1f, 0.1f }, { 0.1f, 0.1f }, { 0.1f, 0.1f }, { 0.1f, 0.1f } }, fgain[kNumFilters] = { 1, 1, 1, 1, 1, 1 };
    float lfoRateMul[2] = { 1, 1 };
    double lfoInc[2] = { 0, 0 }; bool lfoLock[2] = { false, false };
    float fs = 48000.f, smooth = 0.01f, modSmooth = 0.1f, followAtk = 0.1f, followRel = 0.01f;
    float level = 0, vel = 1, chanAT = 0, polyAT = 0, bend = 0, wheel = 0;
    float dcL = 0, dcR = 0, dcXl = 0, dcXr = 0;
    // 3.0
    Diffuser diff[2]; LoFi lofi[2]; PitchShift pitchIn[2], pitchOut[2];
    float dynEnv = 0, dynGain = 1, dynDuck = 1, dynA = 0.01f, dynR = 0.001f, sFreeze = 0, sWidth = 1, sWetL = 1, sWetR = 1, sMainGL = 1, sMainGR = 1;
    float tapTime[kNumTaps] = {}, tapGL[kNumTaps] = {}, tapGR[kNumTaps] = {};
    int ctl = 0;
    int stack[16] = {}; int held = 0, lastNote = -1;
    bool tValid = false, tPlaying = false; double tBarPos = 0, tBpm = 120, tBpb = 4;
};

} // namespace strata
