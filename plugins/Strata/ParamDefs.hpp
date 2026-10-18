// Parameter metadata shared by the DSP wrapper and the UI. License: MIT.
#pragma once
#include "StrataEngine.hpp"
#include <cstdio>
#include <cstring>

namespace strata {

static constexpr uint32_t kHintInteger = 1, kHintBoolean = 2, kHintLog = 4, kHintOutput = 8;
struct ParamDef { const char* name; const char* sym; const char* unit; float min, max; uint32_t hints; };

inline const char* sourceName(int s) {
    static const char* const n[kNumSources] = { "None", "LFO 1", "LFO 2", "Envelope", "Velocity", "Aftertouch",
                                                "Key Track", "Pitch Bend", "Mod Wheel" };
    return n[s < 0 || s >= kNumSources ? 0 : s];
}
inline const char* destName(int d) {
    static const char* const n[kDstSlotFirst] = {
        "None", "Left Time", "Right Time", "Left Pan", "Right Pan", "Left Feedback", "Right Feedback",
        "Cross L>R", "Cross R>L", "Drive",
        "Filter 1 Freq", "Filter 1 Peak", "Filter 1 Pan", "Filter 2 Freq", "Filter 2 Peak", "Filter 2 Pan",
        "Input Gain", "Mix", "Output Gain",
        "LFO 1 Rate", "LFO 1 Balance", "LFO 2 Rate", "LFO 2 Balance",
        "Env Attack", "Env Decay", "Env Sustain", "Env Release" };
    static char slotNames[kNumSlots][24];
    static bool init = false;
    if (!init) { init = true; for (int i = 0; i < kNumSlots; ++i) std::snprintf(slotNames[i], sizeof slotNames[i], "Slot %d Amount", i + 1); }
    if (d < 0 || d >= kNumDests) return n[0];
    return d < kDstSlotFirst ? n[d] : slotNames[d - kDstSlotFirst];
}

inline const ParamDef& paramDef(uint32_t i) {
    static ParamDef t[kParamCount];
    static bool done = false;
    if (!done) {
        done = true;
        auto set = [](uint32_t i, const char* n, const char* s, const char* u, float lo, float hi, uint32_t h) { t[i] = { n, s, u, lo, hi, h }; };
        const uint32_t I = kHintInteger, B = kHintBoolean | kHintInteger;
        set(kTimeL, "Left Time", "timel", "ms", kMinDelayMs, kMaxDelayMs, kHintLog);
        set(kTimeR, "Right Time", "timer", "ms", kMinDelayMs, kMaxDelayMs, kHintLog);
        set(kSyncL, "Left Sync", "syncl", "", 0, 3, I);      set(kSyncR, "Right Sync", "syncr", "", 0, 3, I);
        set(kDivL, "Left Note", "divl", "", 0, kNumNoteDivs - 1, I); set(kDivR, "Right Note", "divr", "", 0, kNumNoteDivs - 1, I);
        set(kOffL, "Left Offset", "offl", "%", -50, 50, 0);  set(kOffR, "Right Offset", "offr", "%", -50, 50, 0);
        set(kLink, "Link", "link", "", 0, 1, B);
        set(kMode, "Mode", "mode", "", 0, 1, I);
        set(kGlide, "Smoothing", "glide", "s", 0.001f, 2, 0);
        set(kPanL, "Left Pan", "panl", "", -1, 1, 0);        set(kPanR, "Right Pan", "panr", "", -1, 1, 0);
        set(kFbL, "Left Feedback", "fbl", "", 0, 1.1f, 0);   set(kFbR, "Right Feedback", "fbr", "", 0, 1.1f, 0);
        set(kXfLR, "Cross L>R", "xflr", "", 0, 1.1f, 0);    set(kXfRL, "Cross R>L", "xfrl", "", 0, 1.1f, 0);
        set(kInvL, "Left Invert", "invl", "", 0, 1, B);      set(kInvR, "Right Invert", "invr", "", 0, 1, B);
        set(kDrive, "Drive", "drive", "", 0, 1, 0);
        set(kInput, "Input", "input", "", 0, 2, I);
        for (int f = 0; f < 2; ++f) {
            const uint32_t b = f == 0 ? kF1On : kF2On;
            static const char* nm[2][7] = { { "Filter 1 On", "Filter 1 Style", "Filter 1 Type", "Filter 1 Slope", "Filter 1 Freq", "Filter 1 Peak", "Filter 1 Pan" },
                                            { "Filter 2 On", "Filter 2 Style", "Filter 2 Type", "Filter 2 Slope", "Filter 2 Freq", "Filter 2 Peak", "Filter 2 Pan" } };
            static const char* sy[2][7] = { { "f1on", "f1style", "f1type", "f1slope", "f1freq", "f1peak", "f1pan" },
                                            { "f2on", "f2style", "f2type", "f2slope", "f2freq", "f2peak", "f2pan" } };
            set(b, nm[f][0], sy[f][0], "", 0, 1, B);
            set(b + 1, nm[f][1], sy[f][1], "", 0, kNumStyles - 1, I);
            set(b + 2, nm[f][2], sy[f][2], "", 0, 2, I);
            set(b + 3, nm[f][3], sy[f][3], "", 0, 2, I);
            set(b + 4, nm[f][4], sy[f][4], "Hz", 20, 20000, kHintLog);
            set(b + 5, nm[f][5], sy[f][5], "", 0, 1, 0);
            set(b + 6, nm[f][6], sy[f][6], "", -1, 1, 0);
        }
        set(kRouting, "Filter Routing", "routing", "", 0, 1, I);
        set(kInGain, "Input Gain", "ingain", "dB", -24, 24, 0);
        set(kMix, "Mix", "mix", "", 0, 1, 0);
        set(kOutGain, "Output Gain", "outgain", "dB", -24, 24, 0);
        for (int l = 0; l < 2; ++l) {
            const uint32_t b = kL1Wave + 6 * l;
            static char nm[2][6][28], sy[2][6][12];
            static const char* f[6] = { "Wave", "Balance", "Sync", "Rate", "Division", "MIDI Retrigger" };
            static const char* fs[6] = { "wave", "bal", "sync", "rate", "div", "retrig" };
            for (int k = 0; k < 6; ++k) { std::snprintf(nm[l][k], 28, "LFO %d %s", l + 1, f[k]); std::snprintf(sy[l][k], 12, "l%d%s", l + 1, fs[k]); }
            set(b, nm[l][0], sy[l][0], "", 0, 4, I); set(b + 1, nm[l][1], sy[l][1], "", 0, 1, 0); set(b + 2, nm[l][2], sy[l][2], "", 0, 3, I);
            set(b + 3, nm[l][3], sy[l][3], "Hz", 0.01f, 50, kHintLog); set(b + 4, nm[l][4], sy[l][4], "", 0, kNumDivs - 1, I); set(b + 5, nm[l][5], sy[l][5], "", 0, 1, B);
        }
        set(kEgAttack, "Env Attack", "egatk", "s", 0, 10, 0);   set(kEgDecay, "Env Decay", "egdec", "s", 0.001f, 10, 0);
        set(kEgSustain, "Env Sustain", "egsus", "", 0, 1, 0);   set(kEgRelease, "Env Release", "egrel", "s", 0.001f, 10, 0);
        set(kEgThreshold, "Env Threshold", "egthr", "dB", -60, 0, 0);
        set(kEgTrigger, "Env Trigger", "egtrig", "", 0, 1, I);
        static char sn[kNumSlots][4][24], ss[kNumSlots][4][12];
        static const char* f[4] = { "Source", "Destination", "Amount", "Bypass" };
        static const char* fs[4] = { "src", "dst", "amt", "byp" };
        for (int s = 0; s < kNumSlots; ++s)
            for (int k = 0; k < 4; ++k) {
                std::snprintf(sn[s][k], 24, "Slot %d %s", s + 1, f[k]); std::snprintf(ss[s][k], 12, "s%d%s", s + 1, fs[k]);
                const float hi = k == 0 ? kNumSources - 1 : (k == 1 ? kNumDests - 1 : 1);
                set(slotParam(s, k), sn[s][k], ss[s][k], "", k == 2 ? -1.f : 0.f, hi, k == 2 ? 0 : (k == 3 ? B : I));
            }
        set(kMidiOn, "MIDI", "midion", "", 0, 1, B);
        set(kOutLearn, "MIDI Learn", "o_learn", "", 0, 100000, kHintOutput);
        set(kOutTimeL, "Live Left Time", "o_timel", "ms", 0, kMaxDelayMs, kHintOutput);
        set(kOutTimeR, "Live Right Time", "o_timer", "ms", 0, kMaxDelayMs, kHintOutput);
        set(kOutEg, "Live Envelope", "o_eg", "", 0, 1, kHintOutput);
    }
    return t[i];
}

inline const char* const* enumLabels(uint32_t p, int& count) {
    static const char* const sync[] = { "Free", "Straight", "Dotted", "Triplet" };
    static const char* const note[] = { "1/64", "1/32", "1/16", "1/8", "1/4", "1/2", "1 bar", "2 bars" };
    static const char* const mode[] = { "Tape", "Stretch" };
    static const char* const style[] = { "Smooth", "Warm", "Monolith", "Hot", "Acid", "Raw" };
    static const char* const type[] = { "Low Pass", "Band Pass", "High Pass" };
    static const char* const slope[] = { "12 dB/oct", "24 dB/oct", "48 dB/oct" };
    static const char* const route[] = { "Serial", "Parallel" };
    static const char* const lw[] = { "Triangle", "Sine", "Square", "Random", "Sample & Hold" };
    static const char* const div[] = { "16 bars", "8 bars", "4 bars", "2 bars", "1 bar", "1/2", "1/4", "1/8", "1/16", "1/32", "1/64" };
    static const char* const trig[] = { "Audio", "MIDI" };
    static const char* const input[] = { "Stereo", "Mono", "Ping-Pong" };
    switch (p) {
    case kSyncL: case kSyncR: case kL1Sync: case kL2Sync: count = 4; return sync;
    case kDivL: case kDivR: count = kNumNoteDivs; return note;
    case kMode: count = 2; return mode;
    case kF1Style: case kF2Style: count = kNumStyles; return style;
    case kF1Type: case kF2Type: count = 3; return type;
    case kF1Slope: case kF2Slope: count = 3; return slope;
    case kRouting: count = 2; return route;
    case kL1Wave: case kL2Wave: count = 5; return lw;
    case kL1Div: case kL2Div: count = kNumDivs; return div;
    case kEgTrigger: count = 2; return trig;
    case kInput: count = 3; return input;
    default: break;
    }
    if (p >= kSlotFirst && p <= kSlotLast) {
        static const char* srcs[kNumSources]; static const char* dsts[kNumDests]; static bool init = false;
        if (!init) { init = true; for (int i = 0; i < kNumSources; ++i) srcs[i] = sourceName(i); for (int i = 0; i < kNumDests; ++i) dsts[i] = destName(i); }
        const int field = (p - kSlotFirst) % 4;
        if (field == kSlotSrc) { count = kNumSources; return srcs; }
        if (field == kSlotDst) { count = kNumDests; return dsts; }
    }
    count = 0; return nullptr;
}

inline bool isTimeParam(uint32_t p) { return p == kEgAttack || p == kEgDecay || p == kEgRelease || p == kGlide; }
// slot amounts use a square-law curve so the small depths a delay time needs are easy to dial in
inline bool isSlotAmount(uint32_t p) { return p >= kSlotFirst && p <= kSlotLast && (p - kSlotFirst) % 4 == kSlotAmt; }
inline float toNorm(uint32_t p, float v) {
    const ParamDef& d = paramDef(p);
    if (isSlotAmount(p)) return 0.5f + 0.5f * std::copysign(std::sqrt(std::fabs(clampf(v, -1.f, 1.f))), v);
    if (d.hints & kHintLog) return std::log(v / d.min) / std::log(d.max / d.min);
    float n = (v - d.min) / (d.max - d.min);
    if (isTimeParam(p)) n = std::sqrt(std::max(n, 0.f));
    return n;
}
inline float fromNorm(uint32_t p, float n) {
    const ParamDef& d = paramDef(p);
    n = clampf(n, 0.f, 1.f);
    if (isSlotAmount(p)) { const float u = 2.f * n - 1.f; return u * std::fabs(u); }
    float v;
    if (d.hints & kHintLog) v = d.min * std::pow(d.max / d.min, n);
    else { if (isTimeParam(p)) n = n * n; v = d.min + n * (d.max - d.min); }
    if (d.hints & kHintInteger) v = std::round(v);
    return v;
}

// which modulation destination a knob maps to (for drag-and-drop), or -1
inline int destForParam(uint32_t p) {
    switch (p) {
    case kTimeL: case kDivL: return kDstTimeL;
    case kTimeR: case kDivR: return kDstTimeR;
    case kPanL: return kDstPanL; case kPanR: return kDstPanR;
    case kFbL: return kDstFbL;   case kFbR: return kDstFbR;
    case kXfLR: return kDstXfLR; case kXfRL: return kDstXfRL;
    case kDrive: return kDstDrive;
    case kF1Freq: return kDstF1Freq; case kF1Peak: return kDstF1Peak; case kF1Pan: return kDstF1Pan;
    case kF2Freq: return kDstF2Freq; case kF2Peak: return kDstF2Peak; case kF2Pan: return kDstF2Pan;
    case kInGain: return kDstInGain; case kMix: return kDstMix; case kOutGain: return kDstOutGain;
    case kL1Rate: case kL1Div: return kDstL1Rate; case kL1Balance: return kDstL1Balance;
    case kL2Rate: case kL2Div: return kDstL2Rate; case kL2Balance: return kDstL2Balance;
    case kEgAttack: return kDstEgAttack; case kEgDecay: return kDstEgDecay; case kEgSustain: return kDstEgSustain; case kEgRelease: return kDstEgRelease;
    default: break;
    }
    if (p >= kSlotFirst && p <= kSlotLast && (p - kSlotFirst) % 4 == kSlotAmt) return kDstSlotFirst + (int)((p - kSlotFirst) / 4);
    return -1;
}

} // namespace strata
