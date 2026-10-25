// Parameter metadata shared by the DSP wrapper and the UI. License: MIT.
#pragma once
#include "StrataEngine.hpp"
#include <cstdio>
#include <cstring>
#include <string>

namespace strata {

static constexpr uint32_t kHintInteger = 1, kHintBoolean = 2, kHintLog = 4, kHintOutput = 8, kHintBipolar = 16;
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

// v2 destinations (the classic ones first, in the same order)
inline const char* destName2(int d) {
    static std::string names[kNumDests2]; static bool init = false;
    if (!init) {
        init = true; char b[48];
        for (int k = 0; k < kDstSlotFirst; ++k) names[k] = std::string(k >= kDstL1Rate ? "Classic " : "") + destName(k);
        names[kV2Freeze] = "Freeze"; names[kV2Lofi] = "Lo-Fi"; names[kV2Diffuse] = "Diffuse"; names[kV2Dyn] = "Dynamics"; names[kV2Pitch] = "Pitch";
        names[kV2Width] = "Stereo Width"; names[kV2WetLevel] = "Wet Level"; names[kV2WetPan] = "Wet Pan";
        for (int f = 0; f < kNumFilters; ++f) { std::snprintf(b, sizeof b, "Filter %d Gain", f + 1); names[kV2GainFirst + f] = b; }
        static const char* const fp3[3] = { "Freq", "Peak", "Pan" };
        for (int f = 2; f < kNumFilters; ++f) for (int k = 0; k < 3; ++k) { std::snprintf(b, sizeof b, "Filter %d %s", f + 1, fp3[k]); names[kV2F36First + (f - 2) * 3 + k] = b; }
        static const char* const tp3[3] = { "Level", "Time", "Pan" };
        for (int t = 0; t < kNumTaps; ++t) for (int k = 0; k < 3; ++k) { std::snprintf(b, sizeof b, "Tap %d %s", t + 1, tp3[k]); names[kV2TapFirst + t * 3 + k] = b; }
        names[kV2MainLevel] = "Main Tap Level"; names[kV2MainPan] = "Main Tap Pan";
        for (int l = 0; l < em::kNumXlfo; ++l) { std::snprintf(b, sizeof b, "XLFO %d Rate", l + 1); names[kV2XlfoRateFirst + l] = b; std::snprintf(b, sizeof b, "XLFO %d Balance", l + 1); names[kV2XlfoBalFirst + l] = b; }
        static const char* const adsr[4] = { "Attack", "Decay", "Sustain", "Release" };
        for (int e = 0; e < em::kNumEg; ++e) for (int k = 0; k < 4; ++k) { std::snprintf(b, sizeof b, "EG %d %s", e + 1, adsr[k]); names[kV2EgFirst + e * 4 + k] = b; }
        for (int s = 0; s < kNumSlots2; ++s) { std::snprintf(b, sizeof b, "Slot %d Amount", s + 1); names[kV2SlotFirst + s] = b; }
    }
    return names[d < 0 || d >= kNumDests2 ? 0 : d].c_str();
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
        set(kInput, "Input", "input", "", 0, 3, I);
        for (int f = 0; f < 2; ++f) {
            const uint32_t b = f == 0 ? kF1On : kF2On;
            static const char* nm[2][7] = { { "Filter 1 On", "Filter 1 Style", "Filter 1 Type", "Filter 1 Slope", "Filter 1 Freq", "Filter 1 Peak", "Filter 1 Pan" },
                                            { "Filter 2 On", "Filter 2 Style", "Filter 2 Type", "Filter 2 Slope", "Filter 2 Freq", "Filter 2 Peak", "Filter 2 Pan" } };
            static const char* sy[2][7] = { { "f1on", "f1style", "f1type", "f1slope", "f1freq", "f1peak", "f1pan" },
                                            { "f2on", "f2style", "f2type", "f2slope", "f2freq", "f2peak", "f2pan" } };
            set(b, nm[f][0], sy[f][0], "", 0, 1, B);
            set(b + 1, nm[f][1], sy[f][1], "", 0, kNumStyles - 1, I);
            set(b + 2, nm[f][2], sy[f][2], "", 0, kNumShapes - 1, I);
            set(b + 3, nm[f][3], sy[f][3], "", 0, kNumSlopes - 1, I);
            set(b + 4, nm[f][4], sy[f][4], "Hz", 20, 20000, kHintLog);
            set(b + 5, nm[f][5], sy[f][5], "", 0, 1, 0);
            set(b + 6, nm[f][6], sy[f][6], "", -1, 1, 0);
        }
        set(kRouting, "Filter Routing", "routing", "", 0, kNumRoutings - 1, I);
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
        set(kOutLearn, "MIDI Learn", "o_learn", "", 0, 10000000, kHintOutput);
        set(kOutTimeL, "Live Left Time", "o_timel", "ms", 0, kMaxDelayMs, kHintOutput);
        set(kOutTimeR, "Live Right Time", "o_timer", "ms", 0, kMaxDelayMs, kHintOutput);
        set(kOutEg, "Live Envelope", "o_eg", "", 0, 1, kHintOutput);
        // ---- 3.0
        static std::string ns[kParamCount], ys[kParamCount];
        auto set3 = [&](uint32_t i, const std::string& n, const std::string& s, const char* u, float lo, float hi, uint32_t h) { ns[i] = n; ys[i] = s; set(i, ns[i].c_str(), ys[i].c_str(), u, lo, hi, h); };
        set3(kChanMode, "Channel Mode", "chmode", "", 0, 1, I);
        set3(kFreeze, "Freeze", "freeze", "", 0, 1, B);
        set3(kWidth, "Stereo Width", "width", "", 0, 1, 0);
        set3(kWetLevel, "Wet Level", "wetlevel", "dB", -36, 12, 0);
        set3(kWetPan, "Wet Pan", "wetpan", "", -1, 1, kHintBipolar);
        set3(kLockMix, "Lock Mix", "lockmix", "", 0, 1, B);
        set3(kLofiOn, "Lo-Fi On", "lofion", "", 0, 1, B);         set3(kLofi, "Lo-Fi", "lofi", "", 0, 1, 0);
        set3(kDiffOn, "Diffuse On", "diffon", "", 0, 1, B);       set3(kDiffuse, "Diffuse", "diffuse", "", 0, 1, 0);
        set3(kDynOn, "Dynamics On", "dynon", "", 0, 1, B);        set3(kDyn, "Dynamics", "dyn", "", -1, 1, kHintBipolar);
        set3(kPitchOn, "Pitch On", "pitchon", "", 0, 1, B);       set3(kPitch, "Pitch", "pitch", "st", -12, 12, kHintBipolar);
        set3(kPitchMirror, "Pitch Mirror", "pitchmirror", "", 0, 1, B);
        set3(kPitchInLoop, "Pitch In Feedback", "pitchloop", "", 0, 1, B);
        set3(kMainOn, "Main Tap On", "mainon", "", 0, 1, B);
        set3(kMainLevel, "Main Tap Level", "mainlevel", "dB", -60, 6, 0);
        set3(kMainPan, "Main Tap Pan", "mainpan", "", -1, 1, kHintBipolar);
        for (int t = 0; t < kNumTaps; ++t) {
            const std::string n = "Tap " + std::to_string(t + 1) + " ", y = "t" + std::to_string(t + 1);
            set3(tpar(t, kTOn), n + "On", y + "on", "", 0, 1, B);
            set3(tpar(t, kTTime), n + "Time", y + "time", "", 0.01f, 1, 0);
            set3(tpar(t, kTLevel), n + "Level", y + "level", "dB", -60, 6, 0);
            set3(tpar(t, kTPan), n + "Pan", y + "pan", "", -1, 1, kHintBipolar);
        }
        for (int f = 0; f < kNumFilters; ++f) set3(fpar(f, kFGain), "Filter " + std::to_string(f + 1) + " Gain", "f" + std::to_string(f + 1) + "gain", "dB", -24, 24, kHintBipolar);
        static const char* const fn[kFBaseFields] = { "On", "Style", "Type", "Slope", "Freq", "Peak", "Pan" };
        static const char* const fsym[kFBaseFields] = { "on", "style", "type", "slope", "freq", "peak", "pan" };
        for (int f = 2; f < kNumFilters; ++f) for (int k = 0; k < kFBaseFields; ++k) {
            const std::string n = "Filter " + std::to_string(f + 1) + " " + fn[k], y = "f" + std::to_string(f + 1) + fsym[k];
            switch (k) {
            case kFOn: set3(fpar(f, k), n, y, "", 0, 1, B); break;
            case kFStyle: set3(fpar(f, k), n, y, "", 0, kNumStyles - 1, I); break;
            case kFType: set3(fpar(f, k), n, y, "", 0, kNumShapes - 1, I); break;
            case kFSlope: set3(fpar(f, k), n, y, "", 0, kNumSlopes - 1, I); break;
            case kFFreq: set3(fpar(f, k), n, y, "Hz", 20, 20000, kHintLog); break;
            case kFPeak: set3(fpar(f, k), n, y, "", 0, 1, 0); break;
            default: set3(fpar(f, k), n, y, "", -1, 1, kHintBipolar); break;
            }
        }
        static const char* const xn[em::kLStep1] = { "Mode", "Wave", "Rate", "Sync", "Division", "Balance", "Glide", "Steps" };
        static const char* const xs[em::kLStep1] = { "mode", "wave", "rate", "sync", "div", "bal", "glide", "steps" };
        for (int l = 0; l < em::kNumXlfo; ++l) for (int f = 0; f < kXlfoFields; ++f) {
            const em::FieldRange r = em::xlfoRange(f); const std::string pre = "XLFO " + std::to_string(l + 1) + " ", sy = "x" + std::to_string(l + 1);
            if (f < em::kLStep1) set3(lp_(l, f), pre + xn[f], sy + xs[f], f == em::kLRate ? "Hz" : "", r.min, r.max, (r.integer ? I : 0) | (r.log ? kHintLog : 0));
            else set3(lp_(l, f), pre + "Step " + std::to_string(f - em::kLStep1 + 1), sy + "s" + std::to_string(f - em::kLStep1 + 1), "", -1, 1, kHintBipolar);
        }
        static const char* const en[kEgFields] = { "Attack", "Decay", "Sustain", "Release", "Trigger", "Threshold" };
        static const char* const es[kEgFields] = { "a", "d", "s", "r", "trig", "thr" };
        static const char* const eu[kEgFields] = { "s", "s", "", "s", "", "dB" };
        for (int e = 0; e < em::kNumEg; ++e) for (int f = 0; f < kEgFields; ++f) { const em::FieldRange r = em::egRange(f);
            set3(ep(e, f), "EG " + std::to_string(e + 1) + " " + en[f], "e" + std::to_string(e + 1) + es[f], eu[f], r.min, r.max, (r.integer ? I : 0) | (r.log ? kHintLog : 0)); }
        static const char* const fn2[kEfFields] = { "Attack", "Release", "Gain", "Source" };
        static const char* const fs2[kEfFields] = { "a", "r", "gain", "src" };
        static const char* const fu2[kEfFields] = { "s", "s", "dB", "" };
        for (int e = 0; e < em::kNumEf; ++e) for (int f = 0; f < kEfFields; ++f) { const em::FieldRange r = em::efRange(f);
            set3(fp(e, f), "EF " + std::to_string(e + 1) + " " + fn2[f], "ef" + std::to_string(e + 1) + fs2[f], fu2[f], r.min, r.max, (r.integer ? I : 0) | (r.log ? kHintLog : 0)); }
        for (int x = 0; x < em::kNumXy; ++x) { set3(xyp(x, 0), "XY " + std::to_string(x + 1) + " X", "xy" + std::to_string(x + 1) + "x", "", 0, 1, 0); set3(xyp(x, 1), "XY " + std::to_string(x + 1) + " Y", "xy" + std::to_string(x + 1) + "y", "", 0, 1, 0); }
        static const char* const sn2[kSlotFields] = { "Source", "Destination", "Amount", "Bypass" };
        static const char* const ss2[kSlotFields] = { "src", "dst", "amt", "byp" };
        for (int s = 0; s < kNumSlots2; ++s) for (int f = 0; f < kSlotFields; ++f) {
            const std::string n = "Mod " + std::to_string(s + 1) + " " + sn2[f], y = "m" + std::to_string(s + 1) + ss2[f];
            if (f == kSlotSrc) set3(sp(s, f), n, y, "", 0, em::kNumSources - 1, I);
            else if (f == kSlotDst) set3(sp(s, f), n, y, "", 0, kNumDests2 - 1, I);
            else if (f == kSlotAmt) set3(sp(s, f), n, y, "", -1, 1, kHintBipolar);
            else set3(sp(s, f), n, y, "", 0, 1, B);
        }
    }
    return t[i];
}

inline const char* const* enumLabels(uint32_t p, int& count) {
    static const char* const sync[] = { "Free", "Straight", "Dotted", "Triplet" };
    static const char* const note[] = { "1/64", "1/32", "1/16", "1/8", "1/4", "1/2", "1 bar", "2 bars" };
    static const char* const mode[] = { "Tape", "Stretch" };
    static const char* const style[] = { "Smooth", "Warm", "Monolith", "Hot", "Acid", "Raw", "Clean", "Tube", "Gentle", "Metal", "Hollow" };
    static const char* const type[] = { "Low Pass", "Band Pass", "High Pass", "Bell", "Low Shelf", "High Shelf", "Notch" };
    static const char* const slope[] = { "12 dB/oct", "24 dB/oct", "48 dB/oct", "6 dB/oct" };
    static const char* const route[] = { "Serial", "Parallel", "Per Channel" };
    static const char* const chm[] = { "Left/Right", "Mid/Side" };
    static const char* const xmode[] = { "Wave", "Steps" };
    static const char* const xwave[] = { "Sine", "Triangle", "Saw Up", "Saw Down", "Square", "Random", "Sample & Hold" };
    static const char* const etrig[] = { "MIDI", "Audio" };
    static const char* const efs[] = { "Input", "Sidechain" };
    static const char* const lw[] = { "Triangle", "Sine", "Square", "Random", "Sample & Hold" };
    static const char* const div[] = { "16 bars", "8 bars", "4 bars", "2 bars", "1 bar", "1/2", "1/4", "1/8", "1/16", "1/32", "1/64" };
    static const char* const trig[] = { "Audio", "MIDI" };
    static const char* const input[] = { "Stereo", "Mono", "Ping-Pong", "Ping-Pong R" };
    switch (p) {
    case kSyncL: case kSyncR: case kL1Sync: case kL2Sync: count = 4; return sync;
    case kDivL: case kDivR: count = kNumNoteDivs; return note;
    case kMode: count = 2; return mode;
    case kRouting: count = kNumRoutings; return route;
    case kChanMode: count = 2; return chm;
    case kL1Wave: case kL2Wave: count = 5; return lw;
    case kL1Div: case kL2Div: count = kNumDivs; return div;
    case kEgTrigger: count = 2; return trig;
    case kInput: count = 4; return input;
    default: break;
    }
    for (int f = 0; f < kNumFilters; ++f) {
        if (p == fpar(f, kFStyle)) { count = kNumStyles; return style; }
        if (p == fpar(f, kFType)) { count = kNumShapes; return type; }
        if (p == fpar(f, kFSlope)) { count = kNumSlopes; return slope; }
    }
    if (p >= kXlfoFirst && p < kEgFirst) {
        const int f = (int)((p - kXlfoFirst) % kXlfoFields);
        if (f == em::kLMode) { count = 2; return xmode; }
        if (f == em::kLWave) { count = em::kNumWaves; return xwave; }
        if (f == em::kLSync) { count = 4; return sync; }
        if (f == em::kLDiv) { count = kNumDivs; return div; }
    }
    if (p >= kEgFirst && p < kEfFirst && (p - kEgFirst) % kEgFields == em::kETrigger) { count = 2; return etrig; }
    if (p >= kEfFirst && p < kXyFirst && (p - kEfFirst) % kEfFields == em::kFSource) { count = 2; return efs; }
    if (p >= kSlot2First && p < kParamCount) {
        static const char* srcs2[em::kNumSources]; static const char* dsts2[kNumDests2]; static bool init2 = false;
        if (!init2) { init2 = true; for (int i = 0; i < em::kNumSources; ++i) srcs2[i] = em::sourceName(i); for (int i = 0; i < kNumDests2; ++i) dsts2[i] = destName2(i); }
        const int f = (int)((p - kSlot2First) % kSlotFields);
        if (f == kSlotSrc) { count = em::kNumSources; return srcs2; }
        if (f == kSlotDst) { count = kNumDests2; return dsts2; }
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
inline bool isSlotAmount(uint32_t p) { return (p >= kSlotFirst && p <= kSlotLast && (p - kSlotFirst) % 4 == kSlotAmt) || (p >= kSlot2First && p < kParamCount && (p - kSlot2First) % kSlotFields == kSlotAmt); }
inline bool isSetting(uint32_t p) { return p < kParamCount && !(paramDef(p).hints & kHintOutput); }
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
// which v2 destination a knob maps to, or -1
inline int destForParam2(uint32_t p) {
    const int c = destForParam(p);
    if (c > 0 && c < kDstSlotFirst) return c;
    switch (p) {
    case kFreeze: return kV2Freeze; case kLofi: return kV2Lofi; case kDiffuse: return kV2Diffuse; case kDyn: return kV2Dyn; case kPitch: return kV2Pitch;
    case kWidth: return kV2Width; case kWetLevel: return kV2WetLevel; case kWetPan: return kV2WetPan; case kMainLevel: return kV2MainLevel; case kMainPan: return kV2MainPan;
    default: break;
    }
    for (int f = 0; f < kNumFilters; ++f) {
        if (p == fpar(f, kFGain)) return kV2GainFirst + f;
        if (f >= 2) { if (p == fpar(f, kFFreq)) return kV2F36First + (f - 2) * 3; if (p == fpar(f, kFPeak)) return kV2F36First + (f - 2) * 3 + 1; if (p == fpar(f, kFPan)) return kV2F36First + (f - 2) * 3 + 2; }
    }
    for (int t = 0; t < kNumTaps; ++t) { if (p == tpar(t, kTLevel)) return kV2TapFirst + t * 3; if (p == tpar(t, kTTime)) return kV2TapFirst + t * 3 + 1; if (p == tpar(t, kTPan)) return kV2TapFirst + t * 3 + 2; }
    for (int l = 0; l < em::kNumXlfo; ++l) { if (p == lp_(l, em::kLRate)) return kV2XlfoRateFirst + l; if (p == lp_(l, em::kLBalance)) return kV2XlfoBalFirst + l; }
    for (int e = 0; e < em::kNumEg; ++e) { static const int f[4] = { em::kEAttack, em::kEDecay, em::kESustain, em::kERelease }; for (int k = 0; k < 4; ++k) if (p == ep(e, f[k])) return kV2EgFirst + e * 4 + k; }
    if (p >= kSlot2First && p < kParamCount && (p - kSlot2First) % kSlotFields == kSlotAmt) return kV2SlotFirst + (int)((p - kSlot2First) / kSlotFields);
    return -1;
}

} // namespace strata
