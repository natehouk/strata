// Strata — custom vector UI (DPF/NanoVG). Original design. License: MIT.
#include "DistrhoUI.hpp"
#include "ParamDefs.hpp"
#include "Presets.hpp"
#include "UserPresets.hpp"
#include "ResizeHandle.hpp"
#include <cctype>
#include <cmath>
#include <complex>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

START_NAMESPACE_DISTRHO

using namespace strata;
using DGL_NAMESPACE::ResizeHandle;
typedef std::complex<float> cplx;

static constexpr float W = 1240.f, H = 780.f;

#define C_BG        Color(0x11, 0x10, 0x0e)
#define C_PANEL     Color(0x1b, 0x19, 0x17)
#define C_PANEL_HI  Color(0x26, 0x23, 0x20)
#define C_LINE      Color(0x33, 0x2f, 0x2a)
#define C_TEXT      Color(0xee, 0xe6, 0xda)
#define C_DIM       Color(0x8e, 0x86, 0x7c)
#define C_FAINT     Color(0x52, 0x4c, 0x45)
#define C_ACC       Color(0xe8, 0xa9, 0x5c)       // sandstone amber
#define C_LEFT      Color(0xe8, 0xa9, 0x5c)
#define C_RIGHT     Color(0x8c, 0xbc, 0xe6)
#define C_DISPLAY   Color(0x0c, 0x0b, 0x0a)

static Color withAlpha(const Color& c, float a) { Color r(c); r.alpha = a; return r; }
static Color srcColor(int s) {
    switch (s) {
    case kSrcLfo1: return Color(0x5f, 0xd0, 0xb4);
    case kSrcLfo2: return Color(0x6f, 0xa8, 0xf0);
    case kSrcEg:   return Color(0xff, 0x86, 0x52);
    case kSrcVelocity: return Color(0xc0, 0x9c, 0xff);
    case kSrcAftertouch: return Color(0xe8, 0x8c, 0xd0);
    case kSrcKey: return Color(0xd6, 0xc8, 0xa6);
    case kSrcBend: return Color(0x9a, 0xa8, 0xb8);
    case kSrcWheel: return Color(0xdc, 0xb4, 0xff);
    default: return C_DIM;
    }
}
static const char* srcShort(int s) {
    static const char* n[kNumSources] = { "-", "LFO 1", "LFO 2", "ENV", "VEL", "AT", "KEY", "BEND", "WHEEL" };
    return n[s < 0 || s >= kNumSources ? 0 : s];
}

// ---------------------------------------------------------------- layout
enum WType { Knob, Mini, Toggle, Segment, Stepper, SlotSrc, SlotDst, SlotByp, SlotDel };
struct Ctl { WType type; uint32_t param; float x, y, r; const char* label; };
struct Panel { float x, y, w, h; const char* title; };
struct Chip { int src; float x, y; };

static constexpr float R1 = 64, R1H = 250, R2 = 322, R2H = 200, R3 = 530, R3H = 234;
static constexpr float MODX = 16, MODW = 868, SLOT_COLW = 281, SLOT_ROWH = 23, SLOT_Y0 = R3 + 42;
static constexpr float LX = 16, EX = 314, RX = 934, SIDEW = 290, ECHOW = 612;
static constexpr float F1X = 16, F2X = 320, FW = 296, LF1X = 624, LF2X = 826, LFW = 194, ENVX = 1028, ENVW = 196, OUTX = 892, OUTW = 332;
static constexpr float DISP_X = EX + 14, DISP_Y = R1 + 34, DISP_W = ECHOW - 28, DISP_H = 150;
static constexpr float TAP_X = EX + 484, TAP_Y = R1 + 205, TAP_W = 114, TAP_H = 26;
static constexpr float CLR2_X = OUTX + 106, CLR2_Y = R3 + 192, CLR2_W = 120, CLR2_H = 24;

static const Panel kPanels[] = {
    { LX, R1, SIDEW, R1H, "LEFT" }, { EX, R1, ECHOW, R1H, "ECHOES" }, { RX, R1, SIDEW, R1H, "RIGHT" },
    { F1X, R2, FW, R2H, "" }, { F2X, R2, FW, R2H, "" },
    { LF1X, R2, LFW, R2H, "LFO 1" }, { LF2X, R2, LFW, R2H, "LFO 2" }, { ENVX, R2, ENVW, R2H, "ENVELOPE" },
    { MODX, R3, MODW, R3H, "MODULATION" }, { OUTX, R3, OUTW, R3H, "OUTPUT" },
};

static std::vector<Ctl> buildControls() {
    std::vector<Ctl> c;
    for (int s = 0; s < 2; ++s) {
        const float x = s == 0 ? LX : RX;
        const uint32_t o = (uint32_t)s;
        c.push_back({ Segment, kSyncL + o, x + 145, R1 + 56, 252, "" });
        c.push_back({ Knob, kTimeL + o, x + 62,  R1 + 128, 30, "TIME" });
        c.push_back({ Knob, kOffL + o,  x + 160, R1 + 128, 17, "OFFSET" });
        c.push_back({ Knob, kPanL + o,  x + 238, R1 + 128, 17, "PAN" });
        c.push_back({ Knob, kFbL + o,   x + 62,  R1 + 210, 20, "FEEDBACK" });
        c.push_back({ Knob, s == 0 ? kXfLR : kXfRL, x + 160, R1 + 210, 17, s == 0 ? "CROSS > R" : "CROSS > L" });
        c.push_back({ Toggle, kInvL + o, x + 238, R1 + 206, 0, "INVERT" });
    }
    c.push_back({ Toggle, kLink, RX + 256, R1 + 20, 0, "LINK" });
    c.push_back({ Segment, kMode, EX + 82, R1 + 218, 132, "" });
    c.push_back({ Knob, kGlide, EX + 192, R1 + 214, 15, "SMOOTH" });
    c.push_back({ Knob, kDrive, EX + 252, R1 + 214, 15, "DRIVE" });
    c.push_back({ Segment, kInput, EX + 382, R1 + 218, 168, "" });
    for (int f = 0; f < 2; ++f) {
        const float x = f == 0 ? F1X : F2X;
        const uint32_t b = f == 0 ? kF1On : kF2On;
        c.push_back({ Toggle, b, x + 30, R2 + 22, 0, f == 0 ? "FILTER 1" : "FILTER 2" });
        c.push_back({ Stepper, b + 1, x + 222, R2 + 22, 124, "" });
        c.push_back({ Segment, b + 2, x + 86, R2 + 52, 140, "" });
        c.push_back({ Segment, b + 3, x + 228, R2 + 52, 120, "" });
        c.push_back({ Knob, b + 4, x + 60,  R2 + 158, 22, "FREQ" });
        c.push_back({ Knob, b + 5, x + 150, R2 + 158, 17, "PEAK" });
        c.push_back({ Knob, b + 6, x + 234, R2 + 158, 17, "PAN" });
    }
    for (int l = 0; l < 2; ++l) {
        const float x = l == 0 ? LF1X : LF2X;
        const uint32_t b = kL1Wave + 6 * l;
        c.push_back({ Segment, b,     x + 97, R2 + 82, 172, "" });
        c.push_back({ Segment, b + 2, x + 97, R2 + 108, 172, "" });
        c.push_back({ Knob, b + 3, x + 38, R2 + 162, 15, "RATE" });
        c.push_back({ Knob, b + 1, x + 97, R2 + 162, 13, "BALANCE" });
        c.push_back({ Toggle, b + 5, x + 158, R2 + 160, 0, "RETRIGGER" });
    }
    c.push_back({ Knob, kEgAttack,    ENVX + 34,  R2 + 104, 13, "ATTACK" });
    c.push_back({ Knob, kEgDecay,     ENVX + 98,  R2 + 104, 13, "DECAY" });
    c.push_back({ Knob, kEgSustain,   ENVX + 162, R2 + 104, 13, "SUSTAIN" });
    c.push_back({ Knob, kEgRelease,   ENVX + 34,  R2 + 160, 13, "RELEASE" });
    c.push_back({ Knob, kEgThreshold, ENVX + 98,  R2 + 160, 13, "THRESH" });
    c.push_back({ Segment, kEgTrigger, ENVX + 162, R2 + 156, 60, "" });
    // output
    c.push_back({ Segment, kRouting, OUTX + 166, R3 + 60, 200, "" });
    c.push_back({ Knob, kInGain,  OUTX + 66,  R3 + 132, 18, "INPUT" });
    c.push_back({ Knob, kMix,     OUTX + 166, R3 + 128, 28, "MIX" });
    c.push_back({ Knob, kOutGain, OUTX + 266, R3 + 132, 18, "OUTPUT" });
    // modulation slots: 3 columns x 8 rows
    for (int s = 0; s < kNumSlots; ++s) {
        const float x = MODX + 12 + (s / 8) * SLOT_COLW, y = SLOT_Y0 + (s % 8) * SLOT_ROWH + SLOT_ROWH / 2;
        c.push_back({ SlotSrc, slotParam(s, kSlotSrc), x + 20, y, 64, "" });
        c.push_back({ SlotDst, slotParam(s, kSlotDst), x + 96, y, 108, "" });
        c.push_back({ Mini,    slotParam(s, kSlotAmt), x + 220, y, 8, "" });
        c.push_back({ SlotByp, slotParam(s, kSlotBypass), x + 242, y, 6, "" });
        c.push_back({ SlotDel, slotParam(s, kSlotSrc), x + 259, y, 6, "" });
    }
    return c;
}
static const std::vector<Ctl>& controls() { static std::vector<Ctl> c = buildControls(); return c; }

static std::vector<Chip> buildChips() {
    std::vector<Chip> c;
    c.push_back({ kSrcLfo1, LF1X + LFW - 62, R2 + 8 });
    c.push_back({ kSrcLfo2, LF2X + LFW - 62, R2 + 8 });
    c.push_back({ kSrcEg,   ENVX + ENVW - 62, R2 + 8 });
    const int g[5] = { kSrcVelocity, kSrcAftertouch, kSrcKey, kSrcBend, kSrcWheel };
    for (int i = 0; i < 5; ++i) c.push_back({ g[i], MODX + 150 + i * 62.f, R3 + 18 });
    return c;
}
static const std::vector<Chip>& chips() { static std::vector<Chip> c = buildChips(); return c; }
static constexpr float CHIP_W = 52, CHIP_H = 18;
static constexpr float CLR_X = MODX + 470, CLR_W = 96;

static const char* const* segLabels(uint32_t p, int& n) {
    static const char* const sync[] = { "FREE", "STRAIGHT", "DOTTED", "TRIPLET" };
    static const char* const lsync[] = { "FREE", "STR", "DOT", "TRIP" };
    static const char* const mode[] = { "TAPE", "STRETCH" };
    static const char* const input[] = { "STEREO", "MONO", "PING-PONG" };
    static const char* const type[] = { "LP", "BP", "HP" };
    static const char* const slope[] = { "12", "24", "48" };
    static const char* const lw[] = { "TRI", "SIN", "SQR", "RND", "S&H" };
    static const char* const route[] = { "SERIAL", "PARALLEL" };
    static const char* const trig[] = { "AUD", "MIDI" };
    switch (p) {
    case kSyncL: case kSyncR: n = 4; return sync;
    case kL1Sync: case kL2Sync: n = 4; return lsync;
    case kMode: n = 2; return mode;
    case kInput: n = 3; return input;
    case kF1Type: case kF2Type: n = 3; return type;
    case kF1Slope: case kF2Slope: n = 3; return slope;
    case kL1Wave: case kL2Wave: n = 5; return lw;
    case kRouting: n = 2; return route;
    case kEgTrigger: n = 2; return trig;
    default: n = 0; return nullptr;
    }
}

static constexpr float PS_X = 484.f, PS_Y = 20.f, PS_W = 272.f, PS_H = 28.f, PS_ARROW = 30.f;
static constexpr float LEARN_X = 782.f, LEARN_W = 72.f, MIDI_X = 862.f, MIDI_W = 56.f;
static constexpr float PM_COLW = 146.f, PM_ROW = 23.f, PM_HEAD = 22.f, PM_PAD = 8.f;

static int slopeStages(float v) { const int s = (int)(v + .5f); return s == 0 ? 1 : (s == 1 ? 2 : 4); }
static cplx filterH(const float* v, int f, float hz) {
    const uint32_t b = f == 0 ? kF1On : kF2On;
    const int type = (int)(v[b + 2] + .5f), stages = slopeStages(v[b + 3]);
    const float fc = v[b + 4], r = std::pow(clampf(v[b + 5], 0.f, 1.f), 0.6f);
    const StyleDef& sd = styleDef((int)(v[b + 1] + .5f));
    const float* kb = bwK(stages);
    const float w = hz / fc;
    cplx h(1, 0);
    for (int i = 0; i < stages; ++i) {
        const float k = std::max(i == stages - 1 ? kb[i] * (1 - r) + sd.kMin * r : kb[i], 0.035f);
        const cplx den(1 - w * w, k * w);
        h *= type == 0 ? cplx(1, 0) / den : (type == 2 ? cplx(-w * w, 0) / den : cplx(0, w * std::max(k, 0.12f)) / den);
    }
    return h;
}
// the combined response of the filters in the loop
static cplx loopH(const float* v, float hz) {
    const bool on1 = v[kF1On] > 0.5f, on2 = v[kF2On] > 0.5f;
    if (!on1 && !on2) return cplx(1, 0);
    if (v[kRouting] > 0.5f) {
        if (on1 && on2) return 0.7071f * (filterH(v, 0, hz) + filterH(v, 1, hz));
        return filterH(v, on1 ? 0 : 1, hz);
    }
    cplx h(1, 0); if (on1) h *= filterH(v, 0, hz); if (on2) h *= filterH(v, 1, hz); return h;
}

class StrataUI : public UI {
public:
    StrataUI() : UI((uint)W, (uint)H), resizeHandle(this) {
        loadSharedResources();
        Engine def;
        for (uint32_t i = 0; i < kParamCount; ++i) { val[i] = def.p[i]; defv[i] = def.p[i]; }
        val[kOutTimeL] = 375; val[kOutTimeR] = 500;
        for (int& m : paramCC) m = -1;
        const double sf = getScaleFactor();
        if (sf != 1.0) setSize((uint)(W * sf), (uint)(H * sf));
        setGeometryConstraints((uint)(W * 0.6 * sf), (uint)(H * 0.6 * sf), true);
        if (isResizable()) resizeHandle.hide();
        userPresets = listUserPresets();
    }

protected:
    float uiScale() const { return (float)getWidth() / W; }

    // ---- values -------------------------------------------------------------
    void setValue(uint32_t p, float v) {
        const ParamDef& d = paramDef(p);
        v = clampf(v, d.min, d.max);
        if (v == val[p]) return;
        val[p] = v; setParameterValue(p, v); repaint();
    }
    void commit(uint32_t p, float v) { editParameter(p, true); setValue(p, v); editParameter(p, false); }

    void parameterChanged(uint32_t i, float v) override {
        if (i >= kParamCount) return;
        if (i == kOutLearn) {
            if (v != lastLearnEcho && v > 0) {
                const int code = (int)v % 10000 - 1, prm = code / 128, cc = code % 128;
                for (int& m : paramCC) if (m == cc) m = -1;
                if (prm >= 0 && prm < (int)kParamCount) paramCC[prm] = cc;
                learnWaiting = -1;
            }
            lastLearnEcho = v; repaint(); return;
        }
        if (val[i] != v) { val[i] = v; repaint(); }
    }
    void stateChanged(const char* key, const char* value) override {
        if (std::strcmp(key, "midimap") != 0) return;
        for (int& m : paramCC) m = -1;
        const char* q = value;
        while (q && *q) {
            int cc = -1, prm = -1;
            if (std::sscanf(q, "%d:%d", &cc, &prm) == 2 && prm >= 0 && prm < (int)kParamCount) paramCC[prm] = cc;
            const char* s = std::strchr(q, ';'); if (!s) break; q = s + 1;
        }
        repaint();
    }

    bool linked() const { return val[kLink] > 0.5f; }
    // a knob that changes meaning: synced time -> note value, synced LFO rate -> division
    uint32_t effParam(const Ctl& c) const {
        if (c.param == kTimeL && val[kSyncL] > 0.5f) return kDivL;
        if (c.param == kTimeR && val[kSyncR] > 0.5f) return kDivR;
        for (int l = 0; l < 2; ++l) { const uint32_t b = kL1Wave + 6 * l; if (c.param == b + 3 && val[b + 2] > 0.5f) return b + 4; }
        return c.param;
    }
    bool dimmed(uint32_t p) const {
        if (linked() && (p == kSyncR || p == kTimeR || p == kDivR || p == kOffR)) return true;
        if ((p == kOffL && val[kSyncL] < 0.5f) || (p == kOffR && val[kSyncR] < 0.5f)) return true;
        if (p >= kF1Style && p <= kF1Pan && val[kF1On] < 0.5f) return true;
        if (p >= kF2Style && p <= kF2Pan && val[kF2On] < 0.5f) return true;
        if (p == kRouting && (val[kF1On] < 0.5f || val[kF2On] < 0.5f)) return true;
        if (p == kEgThreshold && val[kEgTrigger] > 0.5f) return true;
        if (p >= kSlotFirst && p <= kSlotLast) { const int s = (p - kSlotFirst) / 4; if (val[slotParam(s, kSlotBypass)] > 0.5f) return true; }
        return false;
    }
    bool slotUsed(int s) const { return val[slotParam(s, kSlotSrc)] > 0.5f || val[slotParam(s, kSlotDst)] > 0.5f; }

    // ---- formatting ---------------------------------------------------------
    static void fmtMs(float ms, char* b, size_t n) {
        if (ms < 10) snprintf(b, n, "%.2f ms", ms); else if (ms < 100) snprintf(b, n, "%.1f ms", ms);
        else if (ms < 1000) snprintf(b, n, "%.0f ms", ms); else snprintf(b, n, "%.2f s", ms / 1000);
    }
    static bool isPan(uint32_t p) { return p == kPanL || p == kPanR || p == kF1Pan || p == kF2Pan; }
    static bool isGain(uint32_t p) { return p == kInGain || p == kOutGain || p == kEgThreshold; }
    void fmt(uint32_t p, char* b, size_t n) const {
        const float v = val[p];
        int ne = 0;
        if (p == kDivL || p == kDivR) {
            const char* const* l = enumLabels(p, ne); const int sy = (int)(val[p == kDivL ? kSyncL : kSyncR] + .5f);
            snprintf(b, n, "%s%s", l[std::max(0, std::min(ne - 1, (int)std::round(v)))], sy == 2 ? " dot" : (sy == 3 ? " trip" : "")); return;
        }
        if (const char* const* l = enumLabels(p, ne)) { snprintf(b, n, "%s", l[std::max(0, std::min(ne - 1, (int)std::round(v)))]); return; }
        const ParamDef& d = paramDef(p);
        if (d.hints & kHintBoolean) { snprintf(b, n, "%s", v > 0.5f ? "On" : "Off"); return; }
        if (p == kTimeL || p == kTimeR) { fmtMs(v, b, n); return; }
        if (p == kF1Freq || p == kF2Freq) { if (v >= 1000) snprintf(b, n, "%.2f kHz", v / 1000); else snprintf(b, n, "%.0f Hz", v); return; }
        if (isPan(p)) { if (std::fabs(v) < .005f) snprintf(b, n, "Center"); else snprintf(b, n, "%s %.0f%%", v < 0 ? "L" : "R", std::fabs(v) * 100); return; }
        if (p == kOffL || p == kOffR) { snprintf(b, n, "%+.1f%%", v); return; }
        if (isGain(p)) { snprintf(b, n, "%+.1f dB", v); return; }
        if (p == kL1Rate || p == kL2Rate) { snprintf(b, n, v < 1 ? "%.2f Hz" : "%.1f Hz", v); return; }
        if (isTimeParam(p)) { if (v < 1) snprintf(b, n, "%.0f ms", v * 1000); else snprintf(b, n, "%.2f s", v); return; }
        if (p >= kSlotFirst && p <= kSlotLast) { const float a = std::fabs(v) * 100; snprintf(b, n, a < 1 && a > 0 ? "%+.2f%%" : "%+.1f%%", v * 100); return; }
        snprintf(b, n, "%.0f%%", v * 100);
    }
    bool parse(uint32_t p, const std::string& in, float& out) const {
        std::string s; for (char ch : in) if (!std::isspace((unsigned char)ch)) s += (char)std::tolower((unsigned char)ch);
        if (s.empty()) return false;
        int ne = 0;
        if (const char* const* l = enumLabels(p, ne)) {
            for (int i = 0; i < ne; ++i) {
                std::string lab; for (const char* c = l[i]; *c; ++c) if (*c != ' ') lab += (char)std::tolower((unsigned char)*c);
                if (lab.compare(0, s.size(), s) == 0) { out = (float)i; return true; }
            }
            return false;
        }
        if (isPan(p)) {
            if (s[0] == 'c') { out = 0; return true; }
            const float sign = s[0] == 'l' ? -1.f : 1.f;
            out = sign * (float)std::atof((s[0] == 'l' || s[0] == 'r') ? s.c_str() + 1 : s.c_str()) / 100.f; return true;
        }
        char* end = nullptr; double x = std::strtod(s.c_str(), &end);
        if (end == s.c_str()) return false;
        std::string suf(end);
        if (suf == "k" || suf == "khz") x *= 1000;
        if (p == kTimeL || p == kTimeR) { out = (float)(suf == "s" ? x * 1000 : x); return true; }
        const bool raw = p == kF1Freq || p == kF2Freq || p == kL1Rate || p == kL2Rate || isGain(p) || p == kOffL || p == kOffR;
        if (raw) { out = (float)x; return true; }
        if (isTimeParam(p)) { out = (float)(suf == "ms" ? x / 1000 : x); return true; }
        out = (float)(x / 100.0); return true;
    }

    // ---- drawing --------------------------------------------------------------
    void onNanoDisplay() override {
        const float s = uiScale();
        save(); scale(s, s);
        beginPath(); rect(0, 0, W, H); fillColor(C_BG); fill();
        fontFace(NANOVG_DEJAVU_SANS_TTF);

        // header: stacked strata
        const Color bands[4] = { C_ACC, Color(0xc4, 0x7e, 0x46), Color(0x9c, 0x6a, 0x48), Color(0x6e, 0x58, 0x4a) };
        for (int i = 0; i < 4; ++i) {
            beginPath(); const float y0 = 19 + i * 7.5f;
            moveTo(17, y0 + 1.5f * std::sin(i * 1.3f)); bezierTo(24, y0 - 2, 30, y0 + 3, 38, y0 + 0.5f * i);
            lineTo(38, y0 + 6 + 0.5f * i); bezierTo(30, y0 + 9, 24, y0 + 4, 17, y0 + 6 + 1.5f * std::sin(i * 1.3f)); closePath();
            fillColor(bands[i]); fill();
        }
        fontSize(22); textLetterSpacing(9); fillColor(C_TEXT); textAlign(ALIGN_LEFT | ALIGN_MIDDLE); text(50, 34, "STRATA", nullptr);
        fontSize(10); textLetterSpacing(2.5f); fillColor(C_DIM); text(216, 35, "STEREO TAPE DELAY", nullptr);
        textAlign(ALIGN_RIGHT | ALIGN_MIDDLE); text(W - 22, 35, "EPSILON AUDIO  ·  1.0", nullptr);
        drawPresetBar();
        drawHeaderButton(LEARN_X, LEARN_W, learnMode ? "LEARNING" : "LEARN", learnMode, hoverHeader == 1);
        drawHeaderButton(MIDI_X, MIDI_W, "MIDI", val[kMidiOn] > 0.5f, hoverHeader == 2);

        for (const Panel& p : kPanels) drawPanel(p);
        drawSideMarks();
        drawEchoes();
        drawCaption(EX + 82, R1 + 200, "MODE"); drawCaption(EX + 382, R1 + 200, "INPUT");
        drawButton(TAP_X, TAP_Y, TAP_W, TAP_H, tapFlash > 0 ? "TAP" : "TAP TEMPO", tapFlash > 0, hoverTap);
        for (int f = 0; f < 2; ++f) drawFilterCurve(f, (f == 0 ? F1X : F2X) + 14, R2 + 68, FW - 28, 58);
        for (int l = 0; l < 2; ++l) drawLfo(l, (l == 0 ? LF1X : LF2X) + 12, R2 + 34, LFW - 24, 30);
        drawEnvShape(ENVX + 12, R2 + 34, ENVW - 24, 38);
        drawCaption(ENVX + 162, R2 + 174, "TRIGGER");
        drawCaption(OUTX + 166, R3 + 42, "FILTER ROUTING");
        drawButton(CLR2_X, CLR2_Y, CLR2_W, CLR2_H, "CLEAR ECHOES", false, hoverClr2);
        drawSlotBackgrounds();

        const std::vector<Ctl>& cs = controls();
        for (int i = 0; i < (int)cs.size(); ++i) drawCtl(cs[i], i == active || i == hover);
        for (const Chip& c : chips()) drawChip(c.src, c.x, c.y, hoverChip == c.src, false);
        drawStatus();
        if (editing >= 0) drawEditBox();
        if (dragSrc > 0) drawChip(dragSrc, dragPX - CHIP_W / 2, dragPY - CHIP_H / 2, true, true);
        if (popup.open) drawPopup();
        if (menuOpen) drawPresetMenu();
        if (saving) drawSaveBox();
        restore();
        if (tapFlash > 0) { --tapFlash; repaint(); }
    }

    void drawCaption(float cx, float y, const char* t) {
        fontSize(7.5f); textLetterSpacing(1.8f); textAlign(ALIGN_CENTER | ALIGN_MIDDLE); fillColor(C_FAINT); text(cx, y, t, nullptr);
    }
    void drawButton(float x, float y, float w, float h, const char* label, bool on, bool hot) {
        beginPath(); roundedRect(x, y, w, h, 5); fillColor(on ? withAlpha(C_ACC, 0.45f) : (hot ? withAlpha(C_ACC, 0.18f) : C_PANEL_HI)); fill();
        strokeColor(on || hot ? C_ACC : C_LINE); strokeWidth(1); stroke();
        fontSize(9); textLetterSpacing(2.f); textAlign(ALIGN_CENTER | ALIGN_MIDDLE); fillColor(on ? C_TEXT : (hot ? C_TEXT : C_DIM));
        text(x + w / 2, y + h / 2 + 0.5f, label, nullptr);
    }
    void drawHeaderButton(float x, float w, const char* label, bool on, bool hot) {
        beginPath(); roundedRect(x, PS_Y, w, PS_H, 5); fillColor(on ? withAlpha(C_ACC, 0.22f) : C_PANEL); fill();
        strokeColor(on ? C_ACC : (hot ? C_DIM : C_LINE)); strokeWidth(1); stroke();
        fontSize(9.5f); textLetterSpacing(2.f); textAlign(ALIGN_CENTER | ALIGN_MIDDLE); fillColor(on ? C_TEXT : C_DIM);
        text(x + w / 2, PS_Y + PS_H / 2 + 0.5f, label, nullptr);
    }
    void drawPanel(const Panel& p) {
        beginPath(); roundedRect(p.x, p.y, p.w, p.h, 6); fillColor(C_PANEL); fill(); strokeColor(C_LINE); strokeWidth(1); stroke();
        if (p.title[0]) { fontSize(9.5f); textLetterSpacing(2.2f); fillColor(C_DIM); textAlign(ALIGN_LEFT | ALIGN_TOP); text(p.x + 14, p.y + 10, p.title, nullptr); }
    }
    // a coloured tab beside the LEFT / RIGHT titles, and the live time readout
    void drawSideMarks() {
        for (int s = 0; s < 2; ++s) {
            const float x = s == 0 ? LX : RX;
            beginPath(); roundedRect(x + (s == 0 ? 52 : 58), R1 + 11, 16, 7, 3.5f); fillColor(s == 0 ? C_LEFT : C_RIGHT); fill();
            char b[32]; fmtMs(val[s == 0 ? kOutTimeL : kOutTimeR], b, sizeof b);
            fontSize(10); textLetterSpacing(0.8f); textAlign(ALIGN_LEFT | ALIGN_TOP); fillColor(s == 0 ? C_LEFT : C_RIGHT);
            text(x + (s == 0 ? 80 : 86), R1 + 9, b, nullptr);
        }
    }
    void displayBox(float x, float y, float w, float h) {
        beginPath(); roundedRect(x, y, w, h, 4); fillColor(C_DISPLAY); fill(); strokeColor(C_LINE); strokeWidth(1); stroke();
    }

    // Echo map: every repeat of an impulse on each line, with feedback, cross-feedback, invert and the loop filters (at 1 kHz).
    struct Tap { float t, a; int ch; };
    void echoTaps(std::vector<Tap>& out, float window) const {
        const float T[2] = { std::max(val[kOutTimeL], 0.5f), std::max(val[kOutTimeR], 0.5f) };
        const float g = std::min(1.2f, std::abs(loopH(val, 1000.f)));
        const float inv[2] = { val[kInvL] > 0.5f ? -1.f : 1.f, val[kInvR] > 0.5f ? -1.f : 1.f };
        const float fb[2] = { val[kFbL], val[kFbR] }, xf[2] = { val[kXfLR], val[kXfRL] };   // xf[c]: from line c into the other line
        struct Ev { float start, a; int ch; };
        std::vector<Ev> q; const int mode = (int)(val[kInput] + .5f);
        q.push_back({ 0, 1, 0 }); if (mode != 2) q.push_back({ 0, 1, 1 });
        for (size_t i = 0; i < q.size() && out.size() < 400; ++i) {
            const Ev e = q[i]; const float t = e.start + T[e.ch], a = e.a * g;
            if (t > window || std::fabs(a) < 0.015f) continue;
            out.push_back({ t, a, e.ch });
            if (q.size() < 800) {
                if (fb[e.ch] > 0) q.push_back({ t, a * fb[e.ch] * inv[e.ch], e.ch });
                if (xf[e.ch] > 0) q.push_back({ t, a * xf[e.ch] * inv[1 - e.ch], 1 - e.ch });
            }
        }
    }
    void drawEchoes() {
        const float x = DISP_X, y = DISP_Y, w = DISP_W, h = DISP_H, mid = y + h / 2;
        displayBox(x, y, w, h);
        const float maxT = std::max(val[kOutTimeL], val[kOutTimeR]);
        // window: a "nice" length showing several repeats
        static const float nice[] = { 10, 20, 50, 100, 200, 500, 1000, 2000, 4000, 8000, 16000 };
        float window = 16000; for (float n : nice) if (n >= maxT * 4.5f) { window = n; break; }
        // grid
        const float step = window / 10;
        fontSize(7.5f); textLetterSpacing(0.6f); textAlign(ALIGN_CENTER | ALIGN_BOTTOM);
        for (int i = 0; i <= 10; ++i) {
            const float gx = x + 8 + (w - 16) * i / 10.f;
            beginPath(); moveTo(gx, y + 6); lineTo(gx, y + h - 14); strokeColor(withAlpha(C_LINE, i % 5 == 0 ? 0.9f : 0.45f)); strokeWidth(1); stroke();
            if (i % 2 == 0) { char b[16]; const float t = step * i; if (window >= 2000) snprintf(b, sizeof b, "%.1fs", t / 1000); else snprintf(b, sizeof b, "%.0f", t); fillColor(C_FAINT); text(gx, y + h - 3, b, nullptr); }
        }
        beginPath(); moveTo(x + 8, mid); lineTo(x + w - 8, mid); strokeColor(C_LINE); stroke();
        fontSize(7.5f); textLetterSpacing(1.5f); textAlign(ALIGN_LEFT | ALIGN_TOP); fillColor(withAlpha(C_LEFT, 0.7f)); text(x + 10, y + 6, "L", nullptr);
        textAlign(ALIGN_LEFT | ALIGN_BOTTOM); fillColor(withAlpha(C_RIGHT, 0.7f)); text(x + 10, y + h - 16, "R", nullptr);
        // input pulse
        beginPath(); moveTo(x + 8, mid - (h / 2 - 16)); lineTo(x + 8, mid + (h / 2 - 22)); strokeColor(withAlpha(C_TEXT, 0.5f)); strokeWidth(2); stroke();
        std::vector<Tap> taps; echoTaps(taps, window);
        const float amp = h / 2 - 18;
        float peak = 1.f; for (const Tap& t : taps) peak = std::max(peak, std::fabs(t.a));   // resonant loops can exceed unity: rescale
        for (const Tap& t : taps) {
            const float tx = x + 8 + (w - 16) * t.t / window, len = std::fabs(t.a) / peak * amp;
            const Color c = t.ch == 0 ? C_LEFT : C_RIGHT;
            const float y1 = t.ch == 0 ? mid - len : mid + len;
            beginPath(); moveTo(tx, mid); lineTo(tx, y1); strokeColor(withAlpha(c, 0.9f)); strokeWidth(2.2f); lineCap(ROUND); stroke();
            beginPath(); circle(tx, y1, 2.6f); if (t.a < 0) { strokeColor(c); strokeWidth(1.2f); stroke(); } else { fillColor(c); fill(); }
        }
        // readout: times, sync and tempo
        std::string l = sideText(0), r = sideText(1);
        fontSize(9); textLetterSpacing(0.5f); textAlign(ALIGN_RIGHT | ALIGN_TOP);
        fillColor(C_LEFT); text(x + w - 10, y + 6, l.c_str(), nullptr);
        textAlign(ALIGN_RIGHT | ALIGN_BOTTOM); fillColor(C_RIGHT); text(x + w - 10, y + h - 16, r.c_str(), nullptr);
    }
    std::string sideText(int c) const {
        const int s = linked() ? 0 : c;
        char b[64], t[24]; fmtMs(val[c == 0 ? kOutTimeL : kOutTimeR], t, sizeof t);
        const int sy = (int)(val[kSyncL + s] + .5f);
        if (sy == 0) snprintf(b, sizeof b, "%s", t);
        else { char d[24]; fmt(kDivL + s, d, sizeof d); snprintf(b, sizeof b, "%s  ·  %s", d, t); }
        return std::string(c == 1 && linked() ? "LINKED  " : "") + b;
    }
    void drawFilterCurve(int f, float x, float y, float w, float h) {
        displayBox(x, y, w, h);
        const bool on = val[f == 0 ? kF1On : kF2On] > 0.5f;
        const int N = 120;
        beginPath();
        for (int i = 0; i <= N; ++i) {
            const float hz = 20.f * std::pow(1000.f, (float)i / N);
            const float db = clampf(20 * std::log10(std::max(std::abs(filterH(val, f, hz)), 1e-4f)), -30, 22);
            const float px = x + 3 + (w - 6) * i / N, py = y + h - 3 - (h - 6) * (db + 30) / 52;
            if (!i) moveTo(px, py); else lineTo(px, py);
        }
        strokeColor(withAlpha(C_ACC, on ? 0.95f : 0.25f)); strokeWidth(1.5f); stroke();
    }
    void drawEnvShape(float x, float y, float w, float h) {
        displayBox(x, y, w, h);
        const float a = val[kEgAttack], d = val[kEgDecay], s = val[kEgSustain], r = val[kEgRelease];
        const float sus = 0.4f, tot = std::max(a + d + sus + r, 0.3f), sx = (w - 10) / tot, x0 = x + 5, yb = y + h - 4, hh = h - 8;
        auto Y = [&](float lvl) { return yb - hh * lvl; };
        beginPath(); moveTo(x0, Y(0)); lineTo(x0 + a * sx, Y(1));
        for (int i = 1; i <= 16; ++i) { const float t = i / 16.f; lineTo(x0 + (a + d * t) * sx, Y(s + (1 - s) * std::exp(-4 * t))); }
        lineTo(x0 + (a + d + sus) * sx, Y(s));
        for (int i = 1; i <= 16; ++i) { const float t = i / 16.f; lineTo(x0 + (a + d + sus + r * t) * sx, Y(s * std::exp(-4 * t))); }
        strokeColor(srcColor(kSrcEg)); strokeWidth(1.4f); stroke();
        // live level
        const float e = clampf(val[kOutEg], 0.f, 1.f);
        beginPath(); roundedRect(x + w - 8, y + 4, 4, h - 8, 2); fillColor(C_PANEL_HI); fill();
        if (e > 0.002f) { beginPath(); roundedRect(x + w - 8, yb - hh * e, 4, hh * e, 2); fillColor(srcColor(kSrcEg)); fill(); }
    }
    void drawLfo(int l, float x, float y, float w, float h) {
        displayBox(x, y, w, h);
        const uint32_t b = kL1Wave + 6 * l;
        const int wave = (int)(val[b] + .5f);
        beginPath();
        for (int i = 0; i <= 160; ++i) {
            const float t = (float)i / 160, ph = std::fmod(t * 2.f, 1.f);
            const float py = y + h / 2 - Engine::lfoShape(ph, wave, val[b + 1]) * h * 0.34f;
            if (!i) moveTo(x + 4 + (w - 8) * t, py); else lineTo(x + 4 + (w - 8) * t, py);
        }
        strokeColor(srcColor(kSrcLfo1 + l)); strokeWidth(1.4f); stroke();
        char buf[24]; fmt(val[b + 2] > 0.5f ? b + 4 : b + 3, buf, sizeof buf);
        fontSize(8); textLetterSpacing(1.2f); textAlign(ALIGN_RIGHT | ALIGN_TOP); fillColor(C_DIM); text(x + w - 5, y + 3, buf, nullptr);
    }
    void drawChip(int src, float x, float y, bool hot, bool dragging) {
        const Color col = srcColor(src);
        beginPath(); roundedRect(x, y, CHIP_W, CHIP_H, CHIP_H / 2); fillColor(withAlpha(col, dragging ? 0.9f : (hot ? 0.35f : 0.16f))); fill();
        strokeColor(col); strokeWidth(1); stroke();
        beginPath(); circle(x + 9, y + CHIP_H / 2, 3); fillColor(dragging ? C_BG : col); fill();
        fontSize(8); textLetterSpacing(1.2f); textAlign(ALIGN_CENTER | ALIGN_MIDDLE); fillColor(dragging ? C_BG : C_TEXT);
        text(x + CHIP_W / 2 + 5, y + CHIP_H / 2 + 0.5f, srcShort(src), nullptr);
    }
    void drawSlotBackgrounds() {
        for (int s = 0; s < kNumSlots; ++s) {
            const float x = MODX + 12 + (s / 8) * SLOT_COLW, y = SLOT_Y0 + (s % 8) * SLOT_ROWH;
            beginPath(); roundedRect(x, y + 1, SLOT_COLW - 8, SLOT_ROWH - 2, 4);
            fillColor(selSlot == s ? withAlpha(C_ACC, 0.12f) : (slotUsed(s) ? C_PANEL_HI : Color(0x1f, 0x1d, 0x1a))); fill();
            fontSize(8); textLetterSpacing(0.5f); textAlign(ALIGN_RIGHT | ALIGN_MIDDLE); fillColor(C_FAINT);
            char b[4]; snprintf(b, sizeof b, "%d", s + 1); text(x + 14, y + SLOT_ROWH / 2 + 0.5f, b, nullptr);
        }
    }

    // modulation summary for a knob: total |amount| of active slots and the colour of the strongest
    bool modInfo(uint32_t p, float& depth, Color& col) const {
        const int d = destForParam(p);
        if (d < 0) return false;
        depth = 0; float best = 0; int bs = 0;
        for (int s = 0; s < kNumSlots; ++s) {
            if (val[slotParam(s, kSlotBypass)] > 0.5f) continue;
            if ((int)(val[slotParam(s, kSlotDst)] + .5f) != d) continue;
            const int src = (int)(val[slotParam(s, kSlotSrc)] + .5f); if (src <= 0) continue;
            const float a = std::fabs(val[slotParam(s, kSlotAmt)]); depth += a; if (a >= best) { best = a; bs = src; }
        }
        if (depth <= 0) return false;
        col = srcColor(bs); return true;
    }

    void drawKnobBody(float x, float y, float r, float n, bool bipolar, bool hot, bool dim, uint32_t p) {
        const float a0 = 0.75f * 3.14159265f, a1 = 2.25f * 3.14159265f, ang = a0 + (a1 - a0) * n;
        beginPath(); arc(x, y, r, a0, a1, CW); strokeColor(C_LINE); strokeWidth(r > 20 ? 4.f : (r > 12 ? 3.f : 2.5f)); lineCap(ROUND); stroke();
        const float from = bipolar ? (a0 + a1) / 2 : a0;
        Color arcCol = C_ACC;
        if (p == kTimeR || p == kDivR || p == kOffR || p == kPanR || p == kFbR || p == kXfRL) arcCol = C_RIGHT;
        if (std::fabs(ang - from) > 0.001f) { beginPath(); arc(x, y, r, std::min(from, ang), std::max(from, ang), CW); strokeColor(dim ? C_FAINT : arcCol); stroke(); }
        float depth; Color mc;
        if (modInfo(p, depth, mc)) {   // outer ring: this knob is modulated
            const float span = std::max(0.08f, std::min(1.f, depth * 2.f)) * (a1 - a0) * 0.5f;
            beginPath(); arc(x, y, r + 4, std::max(a0, ang - span), std::min(a1, ang + span), CW);
            strokeColor(withAlpha(mc, 0.85f)); strokeWidth(2.f); lineCap(ROUND); stroke();
        }
        const float cr = r - (r > 20 ? 8 : (r > 12 ? 6 : 4));
        beginPath(); circle(x, y, cr);
        fillPaint(radialGradient(x - cr * 0.3f, y - cr * 0.4f, 0, cr * 1.4f, hot ? Color(0x40, 0x3a, 0x34) : C_PANEL_HI, Color(0x15, 0x14, 0x12)));
        fill(); strokeColor(Color(255, 255, 255, 18 / 255.f)); strokeWidth(1); stroke();
        beginPath(); moveTo(x + std::cos(ang) * cr * 0.3f, y + std::sin(ang) * cr * 0.3f); lineTo(x + std::cos(ang) * cr * 0.9f, y + std::sin(ang) * cr * 0.9f);
        strokeColor(dim ? C_DIM : C_TEXT); strokeWidth(r > 12 ? 2.f : 1.5f); lineCap(ROUND); stroke();
    }

    void drawCtl(const Ctl& c, bool hot) {
        const uint32_t p = effParam(c);
        const bool dim = dimmed(p);
        char buf[48];
        const bool dropTarget = dragSrc > 0 && hot && destForParam(p) >= 0;
        switch (c.type) {
        case Knob: case Mini: {
            const bool bip = paramDef(p).min < 0 && p != kEgThreshold;
            if (c.type == Mini && !slotUsed((p - kSlotFirst) / 4)) break;
            drawKnobBody(c.x, c.y, c.r, toNorm(p, val[p]), bip, hot, dim, p);
            if (dropTarget) { beginPath(); circle(c.x, c.y, c.r + 8); strokeColor(srcColor(dragSrc)); strokeWidth(2); stroke(); }
            if (c.type == Knob) {
                fontSize(8.5f); textLetterSpacing(1.5f); textAlign(ALIGN_CENTER | ALIGN_TOP);
                const bool big = c.param == kTimeL || c.param == kTimeR;
                if (big) {        // the time knob always shows its value
                    fmt(p, buf, sizeof buf); fontSize(10); textLetterSpacing(0.6f); fillColor(dim ? C_FAINT : (hot ? C_ACC : C_TEXT)); text(c.x, c.y + c.r + 6, buf, nullptr);
                } else if (hot && editing < 0 && !learnMode && dragSrc <= 0) { fmt(p, buf, sizeof buf); fillColor(C_ACC); text(c.x, c.y + c.r + 6, buf, nullptr); }
                else { fillColor(dim ? C_FAINT : C_DIM); text(c.x, c.y + c.r + 6, c.label, nullptr); }
            }
            break; }
        case Toggle: {
            const bool on = val[p] > 0.5f, isFilterOn = p == kF1On || p == kF2On;
            const float w = 32, h = 17, x = c.x - w / 2, y = c.y - h / 2;
            beginPath(); roundedRect(x, y, w, h, h / 2); fillColor(on && !dim ? withAlpha(C_ACC, 0.25f) : C_PANEL_HI); fill();
            strokeColor(on && !dim ? C_ACC : C_LINE); strokeWidth(1); stroke();
            beginPath(); circle(on ? x + w - h / 2 : x + h / 2, c.y, 5.5f); fillColor(dim ? C_FAINT : (on ? C_ACC : C_DIM)); fill();
            if (isFilterOn) { fontSize(9.5f); textLetterSpacing(2.2f); textAlign(ALIGN_LEFT | ALIGN_MIDDLE); fillColor(on ? C_TEXT : C_DIM); text(c.x + w / 2 + 8, c.y + 0.5f, c.label, nullptr); }
            else if (p == kLink) { fontSize(8); textLetterSpacing(1.4f); textAlign(ALIGN_RIGHT | ALIGN_MIDDLE); fillColor(on ? C_TEXT : C_DIM); text(c.x - w / 2 - 6, c.y + 0.5f, c.label, nullptr); }
            else { fontSize(8); textLetterSpacing(1.4f); textAlign(ALIGN_CENTER | ALIGN_TOP); fillColor(dim ? C_FAINT : C_DIM); text(c.x, c.y + 13, c.label, nullptr); }
            break; }
        case Segment: {
            int n = 0; const char* const* names = segLabels(p, n);
            const int cur = (int)(val[p] + .5f); const float sw = c.r / n, sh = 19, x0 = c.x - c.r / 2;
            for (int i = 0; i < n; ++i) {
                const float x = x0 + sw * i;
                beginPath(); roundedRect(x + 1, c.y - sh / 2, sw - 2, sh, 4); fillColor(i == cur ? (dim ? C_FAINT : C_ACC) : C_PANEL_HI); fill();
                fontSize(8.5f); textLetterSpacing(1.3f); textAlign(ALIGN_CENTER | ALIGN_MIDDLE);
                fillColor(i == cur ? C_BG : (dim ? C_FAINT : C_DIM)); text(x + sw / 2, c.y + 0.5f, names[i], nullptr);
            }
            break; }
        case Stepper: {
            const float x = c.x - c.r / 2, y = c.y - 10, w = c.r, h = 20;
            beginPath(); roundedRect(x, y, w, h, 4); fillColor(C_PANEL_HI); fill(); strokeColor(hot ? C_DIM : C_LINE); strokeWidth(1); stroke();
            for (int side = 0; side < 2; ++side) {
                const float ax = side == 0 ? x + 10 : x + w - 10, d = side == 0 ? -1.f : 1.f;
                beginPath(); moveTo(ax - 2.5f * d, c.y - 4); lineTo(ax + 2.5f * d, c.y); lineTo(ax - 2.5f * d, c.y + 4); closePath(); fillColor(dim ? C_FAINT : C_DIM); fill();
            }
            fmt(p, buf, sizeof buf); std::string up(buf); for (char& ch : up) ch = (char)std::toupper((unsigned char)ch);
            fontSize(9.5f); textLetterSpacing(1.2f); textAlign(ALIGN_CENTER | ALIGN_MIDDLE); fillColor(dim ? C_FAINT : C_ACC); text(c.x, c.y + 0.5f, up.c_str(), nullptr);
            break; }
        case SlotSrc: case SlotDst: {
            const int s = (p - kSlotFirst) / 4;
            if (!slotUsed(s)) { if (c.type == SlotSrc) { fontSize(9); textLetterSpacing(0.6f); textAlign(ALIGN_LEFT | ALIGN_MIDDLE); fillColor(C_FAINT); text(c.x, c.y + 0.5f, "empty", nullptr); } break; }
            const int v = (int)(val[p] + .5f);
            if (c.type == SlotSrc) {
                beginPath(); circle(c.x + 4, c.y, 3); fillColor(srcColor(v)); fill();
                fontSize(9); textLetterSpacing(0.6f); textAlign(ALIGN_LEFT | ALIGN_MIDDLE); fillColor(hot ? C_TEXT : (dim ? C_FAINT : srcColor(v)));
                text(c.x + 11, c.y + 0.5f, srcShort(v), nullptr);
                fillColor(C_FAINT); text(c.x + 58, c.y + 0.5f, ">", nullptr);
            } else {
                std::string nm = destName(v); if (nm.size() > 17) nm = nm.substr(0, 16) + ".";
                fontSize(9); textLetterSpacing(0.4f); textAlign(ALIGN_LEFT | ALIGN_MIDDLE); fillColor(hot ? C_TEXT : (dim ? C_FAINT : C_DIM));
                text(c.x - 22, c.y + 0.5f, nm.c_str(), nullptr);
            }
            break; }
        case SlotByp: {
            const int s = (p - kSlotFirst) / 4; if (!slotUsed(s)) break;
            const bool byp = val[p] > 0.5f;
            beginPath(); circle(c.x, c.y, 5); fillColor(byp ? C_PANEL : withAlpha(C_ACC, 0.8f)); fill(); strokeColor(byp ? C_FAINT : C_ACC); strokeWidth(1); stroke();
            break; }
        case SlotDel: {
            const int s = (p - kSlotFirst) / 4; if (!slotUsed(s)) break;
            beginPath(); moveTo(c.x - 3.5f, c.y - 3.5f); lineTo(c.x + 3.5f, c.y + 3.5f); moveTo(c.x + 3.5f, c.y - 3.5f); lineTo(c.x - 3.5f, c.y + 3.5f);
            strokeColor(hot ? C_TEXT : C_FAINT); strokeWidth(1.3f); stroke();
            break; }
        }
        // MIDI learn badges
        if (learnMode && (c.type == Knob || c.type == Mini || c.type == Toggle || c.type == Segment || c.type == Stepper)) {
            const int cc = paramCC[c.param]; const bool waiting = learnWaiting == (int)c.param;
            if (cc >= 0 || waiting) {
                const float by = c.type == Knob ? c.y - c.r - 10 : c.y - 15;
                snprintf(buf, sizeof buf, waiting ? "?" : "CC%d", cc);
                fontSize(8); textLetterSpacing(0.5f); textAlign(ALIGN_CENTER | ALIGN_MIDDLE);
                const float bw = waiting ? 14 : 30;
                beginPath(); roundedRect(c.x - bw / 2, by - 6, bw, 12, 3); fillColor(waiting ? C_ACC : C_PANEL_HI); fill(); strokeColor(C_ACC); strokeWidth(1); stroke();
                fillColor(waiting ? C_BG : C_ACC); text(c.x, by + 0.5f, buf, nullptr);
            }
        }
    }

    void drawStatus() {
        std::string msg; char vb[48];
        if (statusFlash > 0) msg = statusText;
        else if (learnMode) msg = learnWaiting >= 0 ? std::string("Move a MIDI controller: ") + paramDef(learnWaiting).name : "MIDI Learn: click a control, move a controller. Right-click unassigns.";
        else if (dragSrc > 0) { const int h = hover; msg = std::string("Drop ") + sourceName(dragSrc) + (h >= 0 && destForParam(effParam(controls()[h])) >= 0 ? std::string(" onto ") + destName(destForParam(effParam(controls()[h]))) : " onto a knob"); }
        else if (hoverTap) msg = "Click in time to set the delay. The right side keeps its ratio to the left.";
        else if (hoverClr2) msg = "Silence everything still echoing";
        else if (hover >= 0) {
            const Ctl& c = controls()[hover]; const uint32_t p = effParam(c);
            if (c.type == SlotDel) msg = "Remove this modulation";
            else if (c.type == SlotByp) msg = "Bypass this modulation";
            else { fmt(p, vb, sizeof vb); msg = std::string(paramDef(p).name) + "   " + vb; }
        } else msg = "Drag a source onto any knob to modulate it";
        fontSize(9); textLetterSpacing(0.6f); textAlign(ALIGN_RIGHT | ALIGN_MIDDLE); fillColor(learnMode || statusFlash > 0 ? C_ACC : C_DIM);
        text(MODX + MODW - 14, R3 + 27, msg.c_str(), nullptr);
        if (learnMode && learnWaiting < 0) {
            beginPath(); roundedRect(CLR_X, R3 + 9, CLR_W, CHIP_H, 4); fillColor(hoverClear ? withAlpha(C_ACC, 0.25f) : C_PANEL_HI); fill(); strokeColor(C_ACC); strokeWidth(1); stroke();
            fontSize(8); textLetterSpacing(1.2f); textAlign(ALIGN_CENTER | ALIGN_MIDDLE); fillColor(C_TEXT); text(CLR_X + CLR_W / 2, R3 + 9 + CHIP_H / 2 + 0.5f, "CLEAR ALL CCs", nullptr);
        }
        if (statusFlash > 0) --statusFlash;
    }
    void flash(const std::string& s) { statusText = s; statusFlash = 120; repaint(); }

    void drawEditBox() {
        const Ctl& c = controls()[editing];
        const float w = 84, h = 20, x = c.x - w / 2, y = c.y + c.r + 3;
        beginPath(); roundedRect(x, y, w, h, 4); fillColor(C_DISPLAY); fill(); strokeColor(C_ACC); strokeWidth(1); stroke();
        fontSize(11); textLetterSpacing(0.5f); textAlign(ALIGN_CENTER | ALIGN_MIDDLE); fillColor(C_TEXT); text(c.x, y + h / 2 + 0.5f, (editBuf + "|").c_str(), nullptr);
    }

    // ---- slot pickers (source / destination lists) ---------------------------
    struct Popup { bool open = false; uint32_t param = 0; float x = 0, y = 0; int hover = -1; } popup;
    int popupCount() const { return (popup.param - kSlotFirst) % 4 == kSlotSrc ? (int)kNumSources : (int)kNumDests; }
    const char* popupLabel(int i) const { return (popup.param - kSlotFirst) % 4 == kSlotSrc ? sourceName(i) : destName(i); }
    void popupLayout(float& x, float& y, float& w, float& h, int& rows, float& colw) const {
        const int n = popupCount(); rows = std::min(n, 14); const int cols = (n + rows - 1) / rows; colw = 118;
        w = cols * colw + 12; h = rows * 19 + 12;
        x = std::min(W - w - 8, std::max(8.f, popup.x)); y = std::min(H - h - 8, std::max(8.f, popup.y));
    }
    void drawPopup() {
        float x, y, w, h, colw; int rows; popupLayout(x, y, w, h, rows, colw);
        beginPath(); rect(x - 10, y - 4, w + 20, h + 18); fillPaint(boxGradient(x, y + 2, w, h, 6, 12, Color(0, 0, 0, 0.6f), Color(0, 0, 0, 0.f))); fill();
        beginPath(); roundedRect(x, y, w, h, 6); fillColor(C_PANEL_HI); fill(); strokeColor(C_LINE); strokeWidth(1); stroke();
        const int cur = (int)(val[popup.param] + .5f);
        const bool isSrc = (popup.param - kSlotFirst) % 4 == kSlotSrc;
        for (int i = 0; i < popupCount(); ++i) {
            const float ix = x + 6 + (i / rows) * colw, iy = y + 6 + (i % rows) * 19;
            if (i == popup.hover) { beginPath(); roundedRect(ix, iy, colw - 4, 19, 3); fillColor(withAlpha(C_ACC, 0.25f)); fill(); }
            if (isSrc && i > 0) { beginPath(); circle(ix + 7, iy + 9.5f, 3); fillColor(srcColor(i)); fill(); }
            fontSize(9.5f); textLetterSpacing(0.3f); textAlign(ALIGN_LEFT | ALIGN_MIDDLE); fillColor(i == cur ? C_ACC : (i == popup.hover ? C_TEXT : C_DIM));
            text(ix + (isSrc ? 15 : 6), iy + 10, popupLabel(i), nullptr);
        }
    }
    int popupHit(float px, float py) const {
        float x, y, w, h, colw; int rows; popupLayout(x, y, w, h, rows, colw);
        if (px < x + 6 || py < y + 6 || px > x + w - 6 || py > y + h - 6) return -1;
        const int i = (int)((px - x - 6) / colw) * rows + (int)((py - y - 6) / 19);
        return i < popupCount() ? i : -1;
    }

    // ---- presets ------------------------------------------------------------------
    bool matchesValues(const float* v) const {
        for (uint32_t i = 0; i < kParamCount; ++i) {
            if (paramDef(i).hints & kHintOutput) continue;
            if (std::fabs(val[i] - v[i]) > 1e-4f * std::max(1.f, std::fabs(v[i]))) return false;
        }
        return true;
    }
    void applyValues(const float* v) { for (uint32_t i = 0; i < kParamCount; ++i) if (!(paramDef(i).hints & kHintOutput) && val[i] != v[i]) commit(i, v[i]); }
    void applyFactory(int idx) { idx = (idx % (int)kNumPresets + (int)kNumPresets) % (int)kNumPresets; applyValues(presets()[idx].v); curFactory = idx; curUser.clear(); repaint(); }
    void applyUser(const std::string& name) { float v[kParamCount]; if (!loadUserPreset(name, v)) return; applyValues(v); std::copy(v, v + kParamCount, userVals); curUser = name; curFactory = -1; repaint(); }
    void programLoaded(uint32_t index) override { curFactory = (int)index; curUser.clear(); repaint(); }
    void step(int d) {
        const int nf = (int)kNumPresets, nu = (int)userPresets.size(), total = nf + nu;
        int cur = curFactory; if (!curUser.empty()) for (int i = 0; i < nu; ++i) if (userPresets[i] == curUser) cur = nf + i;
        if (cur < 0) cur = 0;
        const int next = ((cur + d) % total + total) % total;
        if (next < nf) applyFactory(next); else applyUser(userPresets[next - nf]);
    }
    void drawPresetBar() {
        const float x = PS_X, y = PS_Y, w = PS_W, h = PS_H, a = PS_ARROW;
        beginPath(); roundedRect(x, y, w, h, 5); fillColor(C_PANEL); fill(); strokeColor(menuOpen ? C_ACC : C_LINE); strokeWidth(1); stroke();
        beginPath(); moveTo(x + a, y + 5); lineTo(x + a, y + h - 5); moveTo(x + w - a, y + 5); lineTo(x + w - a, y + h - 5); strokeColor(C_LINE); stroke();
        for (int side = 0; side < 2; ++side) {
            const bool hot = presetHover == (side == 0 ? 0 : 2);
            const float cx = side == 0 ? x + a / 2 : x + w - a / 2, cy = y + h / 2, d = side == 0 ? -1.f : 1.f;
            beginPath(); moveTo(cx - 3 * d, cy - 5); lineTo(cx + 3 * d, cy); lineTo(cx - 3 * d, cy + 5); closePath(); fillColor(hot ? C_ACC : C_DIM); fill();
        }
        std::string name; bool modified = false;
        if (!curUser.empty()) { name = curUser; modified = !matchesValues(userVals); }
        else if (curFactory >= 0) { name = presets()[curFactory].name; modified = !matchesValues(presets()[curFactory].v); }
        else name = "Custom";
        if (modified) name += " *";
        fontSize(12); textLetterSpacing(1.2f); textAlign(ALIGN_CENTER | ALIGN_MIDDLE); fillColor(presetHover == 1 || menuOpen ? C_TEXT : Color(0xd6, 0xce, 0xc2));
        text(x + w / 2 - 6, y + h / 2 + 0.5f, name.c_str(), nullptr);
        const float cx = x + w - a - 14, cy = y + h / 2;
        beginPath(); moveTo(cx - 4, cy - 2); lineTo(cx + 4, cy - 2); lineTo(cx, cy + 3); closePath(); fillColor(C_DIM); fill();
    }
    struct MRect { float x, y, w, h; int kind; int idx; };
    void menuLayout(float& mx, float& my, float& mw, float& mh, std::vector<MRect>& items) const {
        const int cols = kNumCategories + 1;
        mw = PM_COLW * cols + 2 * PM_PAD; mx = std::min(W - mw - 8, std::max(8.f, PS_X + PS_W / 2 - mw / 2)); my = PS_Y + PS_H + 4;
        items.clear();
        const float top = my + PM_PAD + PM_ROW + 6 + PM_HEAD;
        int rows[kNumCategories + 1] = {}; int maxRows = 1;
        for (uint32_t i = 0; i < kNumPresets; ++i) {
            if (i == 0) { items.push_back({ mx + PM_PAD, my + PM_PAD, PM_COLW - 4, PM_ROW, 0, 0 }); continue; }   // Init sits on the top row
            const char* cat = presets()[i].category;
            for (int c = 0; c < kNumCategories; ++c)
                if (std::strcmp(cat, kCategories[c]) == 0) { items.push_back({ mx + PM_PAD + c * PM_COLW, top + rows[c] * PM_ROW, PM_COLW - 4, PM_ROW, 0, (int)i }); maxRows = std::max(maxRows, ++rows[c]); }
        }
        for (int u = 0; u < (int)userPresets.size() && u < 16; ++u) items.push_back({ mx + PM_PAD + kNumCategories * PM_COLW, top + u * PM_ROW, PM_COLW - 4, PM_ROW, 1, u });
        maxRows = std::max(maxRows, std::min(16, (int)userPresets.size()));
        items.push_back({ mx + mw - PM_PAD - 150, my + PM_PAD, 150, PM_ROW, 2, 0 });
        mh = PM_PAD + PM_ROW + 6 + PM_HEAD + maxRows * PM_ROW + PM_PAD;
    }
    void drawPresetMenu() {
        float mx, my, mw, mh; std::vector<MRect> items; menuLayout(mx, my, mw, mh, items);
        beginPath(); rect(mx - 12, my - 4, mw + 24, mh + 20); fillPaint(boxGradient(mx, my + 2, mw, mh, 6, 14, Color(0, 0, 0, 0.6f), Color(0, 0, 0, 0.f))); fill();
        beginPath(); roundedRect(mx, my, mw, mh, 6); fillColor(C_PANEL_HI); fill(); strokeColor(C_LINE); strokeWidth(1); stroke();
        beginPath(); moveTo(mx + PM_PAD, my + PM_PAD + PM_ROW + 3); lineTo(mx + mw - PM_PAD, my + PM_PAD + PM_ROW + 3); strokeColor(C_LINE); stroke();
        fontSize(8.5f); textLetterSpacing(2.f); textAlign(ALIGN_LEFT | ALIGN_MIDDLE);
        for (int c = 0; c <= kNumCategories; ++c) {
            std::string up = c < kNumCategories ? kCategories[c] : "User"; for (char& ch : up) ch = (char)std::toupper((unsigned char)ch);
            fillColor(c < kNumCategories ? C_ACC : C_RIGHT); text(mx + PM_PAD + c * PM_COLW + 12, my + PM_PAD + PM_ROW + 6 + PM_HEAD / 2, up.c_str(), nullptr);
        }
        if (userPresets.empty()) { fontSize(10); textLetterSpacing(0.4f); fillColor(C_FAINT); text(mx + PM_PAD + kNumCategories * PM_COLW + 12, my + PM_PAD + PM_ROW + 6 + PM_HEAD + PM_ROW / 2, "(none saved yet)", nullptr); }
        for (int i = 0; i < (int)items.size(); ++i) {
            const MRect& r = items[i]; const bool hot = i == menuHover;
            if (r.kind == 2) {
                beginPath(); roundedRect(r.x, r.y + 1, r.w, r.h - 2, 4); fillColor(hot ? withAlpha(C_ACC, 0.25f) : C_PANEL); fill(); strokeColor(C_ACC); strokeWidth(1); stroke();
                fontSize(9.5f); textLetterSpacing(1.6f); textAlign(ALIGN_CENTER | ALIGN_MIDDLE); fillColor(C_TEXT); text(r.x + r.w / 2, r.y + r.h / 2 + 0.5f, "SAVE CURRENT AS...", nullptr);
                continue;
            }
            const bool isCur = r.kind == 0 ? (curUser.empty() && r.idx == curFactory) : (curUser == userPresets[r.idx]);
            const bool del = r.kind == 1 && pendingDelete == r.idx;
            if (hot || del) { beginPath(); roundedRect(r.x, r.y, r.w, r.h, 4); fillColor(del ? Color(0xd0, 0x50, 0x40, 0.35f) : withAlpha(C_ACC, 0.2f)); fill(); }
            if (isCur) { beginPath(); circle(r.x + 6, r.y + r.h / 2, 2.5f); fillColor(C_ACC); fill(); }
            fontSize(11); textLetterSpacing(0.5f); textAlign(ALIGN_LEFT | ALIGN_MIDDLE); fillColor(hot || isCur || del ? C_TEXT : C_DIM);
            const std::string label = del ? "Delete? click again" : (r.kind == 0 ? std::string(presets()[r.idx].name) : userPresets[r.idx]);
            text(r.x + 12, r.y + r.h / 2 + 0.5f, label.c_str(), nullptr);
        }
    }
    int menuHit(float x, float y) const { float mx, my, mw, mh; std::vector<MRect> it; menuLayout(mx, my, mw, mh, it);
        for (int i = 0; i < (int)it.size(); ++i) if (x >= it[i].x && x < it[i].x + it[i].w && y >= it[i].y && y < it[i].y + it[i].h) return i; return -1; }
    MRect menuItem(int i) const { float mx, my, mw, mh; std::vector<MRect> it; menuLayout(mx, my, mw, mh, it); return it[i]; }
    void drawSaveBox() {
        const float w = 360, h = 96, x = W / 2 - w / 2, y = 140;
        beginPath(); rect(0, 0, W, H); fillColor(Color(0, 0, 0, 0.45f)); fill();
        beginPath(); roundedRect(x, y, w, h, 8); fillColor(C_PANEL_HI); fill(); strokeColor(C_ACC); strokeWidth(1); stroke();
        fontSize(9.5f); textLetterSpacing(2.f); textAlign(ALIGN_LEFT | ALIGN_TOP); fillColor(C_DIM); text(x + 16, y + 14, "SAVE PRESET AS", nullptr);
        beginPath(); roundedRect(x + 16, y + 36, w - 32, 26, 4); fillColor(C_DISPLAY); fill(); strokeColor(C_LINE); stroke();
        fontSize(13); textLetterSpacing(0.4f); textAlign(ALIGN_LEFT | ALIGN_MIDDLE); fillColor(C_TEXT); text(x + 26, y + 49.5f, (saveName + "|").c_str(), nullptr);
        fontSize(8.5f); textLetterSpacing(1.2f); fillColor(C_FAINT); textAlign(ALIGN_RIGHT | ALIGN_TOP);
        text(x + w - 16, y + 72, saveError ? "COULD NOT SAVE - CHECK THE NAME" : "ENTER TO SAVE   ·   ESC TO CANCEL", nullptr);
    }

    // ---- tap tempo -------------------------------------------------------------------
    void tap(uint time) {
        const double now = time;
        if (nTaps > 0 && (now - taps[nTaps - 1] > 2500 || now <= taps[nTaps - 1])) nTaps = 0;
        if (nTaps == 5) { for (int i = 0; i < 4; ++i) taps[i] = taps[i + 1]; nTaps = 4; }
        taps[nTaps++] = now;
        tapFlash = 6;
        if (nTaps < 2) { flash("Tap again..."); return; }
        const float ms = clampf((float)((taps[nTaps - 1] - taps[0]) / (nTaps - 1)), kMinDelayMs, kMaxDelayMs);
        // keep the right side's ratio to the left, using the unmodulated times (synced sides: their current length)
        const float baseL = val[kSyncL] > 0.5f ? val[kOutTimeL] : val[kTimeL], baseR = val[kSyncR] > 0.5f ? val[kOutTimeR] : val[kTimeR];
        const float ratio = linked() ? 1.f : std::max(baseR, 0.5f) / std::max(baseL, 0.5f);
        commit(kSyncL, 0); commit(kTimeL, ms);
        if (!linked()) { commit(kSyncR, 0); commit(kTimeR, clampf(ms * ratio, kMinDelayMs, kMaxDelayMs)); }
        val[kOutTimeL] = ms; if (!linked()) val[kOutTimeR] = ms * ratio;
        char b[64]; snprintf(b, sizeof b, "Tap tempo: %.0f ms  (%.1f BPM)", ms, 60000.f / ms); flash(b);
    }

    // ---- interaction --------------------------------------------------------------
    int hit(float x, float y, int* seg = nullptr) const {
        const std::vector<Ctl>& cs = controls();
        for (int i = 0; i < (int)cs.size(); ++i) {
            const Ctl& c = cs[i];
            switch (c.type) {
            case Knob: case Mini: {
                if (c.type == Mini && !slotUsed((c.param - kSlotFirst) / 4)) break;
                const float rr = c.r + (c.type == Mini ? 4 : 6); if ((x - c.x) * (x - c.x) + (y - c.y) * (y - c.y) <= rr * rr) return i; break; }
            case Toggle: if (std::fabs(x - c.x) <= 19 && std::fabs(y - c.y) <= 11) return i; break;
            case Segment: { int n = 0; segLabels(c.param, n); const float x0 = c.x - c.r / 2;
                if (x >= x0 && x < x0 + c.r && std::fabs(y - c.y) <= 10) { if (seg) *seg = (int)((x - x0) / (c.r / n)); return i; } break; }
            case Stepper: if (std::fabs(x - c.x) <= c.r / 2 && std::fabs(y - c.y) <= 11) { if (seg) *seg = x < c.x ? -1 : 1; return i; } break;
            case SlotSrc: if (x >= c.x - 4 && x < c.x + c.r && std::fabs(y - c.y) <= 10) return i; break;
            case SlotDst: { const int s = (c.param - kSlotFirst) / 4; if (!slotUsed(s)) break;
                if (x >= c.x - 24 && x < c.x + c.r - 22 && std::fabs(y - c.y) <= 10) return i; break; }
            case SlotByp: case SlotDel: { const int s = (c.param - kSlotFirst) / 4; if (!slotUsed(s)) break;
                if (std::fabs(x - c.x) <= 8 && std::fabs(y - c.y) <= 9) return i; break; }
            }
        }
        return -1;
    }
    static bool inRect(float x, float y, float rx, float ry, float rw, float rh) { return x >= rx && x < rx + rw && y >= ry && y < ry + rh; }
    bool clearHit(float x, float y) const { return learnMode && learnWaiting < 0 && inRect(x, y, CLR_X, R3 + 9, CLR_W, CHIP_H); }
    int chipHit(float x, float y) const { for (const Chip& c : chips()) if (inRect(x, y, c.x, c.y, CHIP_W, CHIP_H)) return c.src; return -1; }
    int headerHit(float x, float y) const {
        if (y < PS_Y || y > PS_Y + PS_H) return 0;
        if (x >= LEARN_X && x <= LEARN_X + LEARN_W) return 1;
        if (x >= MIDI_X && x <= MIDI_X + MIDI_W) return 2;
        return 0;
    }
    int presetBarHit(float x, float y) const {
        if (y < PS_Y || y > PS_Y + PS_H || x < PS_X || x > PS_X + PS_W) return -1;
        return x < PS_X + PS_ARROW ? 0 : (x > PS_X + PS_W - PS_ARROW ? 2 : 1);
    }

    void assign(int src, int dst) {
        int slot = -1;
        for (int s = 0; s < kNumSlots; ++s) if (!slotUsed(s)) { slot = s; break; }
        if (slot < 0) { flash("All 24 modulation slots are in use - remove one first"); return; }
        const float amt = (dst == kDstTimeL || dst == kDstTimeR) ? (src == kSrcKey ? -1.f : 0.005f)
                        : ((src == kSrcKey && (dst == kDstF1Freq || dst == kDstF2Freq)) ? 0.2f : 0.3f);
        commit(slotParam(slot, kSlotSrc), (float)src); commit(slotParam(slot, kSlotDst), (float)dst);
        commit(slotParam(slot, kSlotAmt), amt); commit(slotParam(slot, kSlotBypass), 0);
        selSlot = slot;
        flash(std::string("Slot ") + std::to_string(slot + 1) + ": " + sourceName(src) + " > " + destName(dst));
    }
    void clearSlot(int s) {
        commit(slotParam(s, kSlotSrc), 0); commit(slotParam(s, kSlotDst), 0); commit(slotParam(s, kSlotAmt), 0); commit(slotParam(s, kSlotBypass), 0);
    }
    void setLearn(bool on) { learnMode = on; learnWaiting = -1; if (!on) setState("cmd", "cancel"); repaint(); }
    void endEdit(bool apply) {
        if (editing < 0) return;
        const uint32_t p = effParam(controls()[editing]); float v;
        if (apply && parse(p, editBuf, v)) commit(p, v);
        editing = -1; editBuf.clear(); repaint();
    }

    bool onKeyboard(const KeyboardEvent& ev) override {
        if (!ev.press) return false;
        std::string* buf = saving ? &saveName : (editing >= 0 ? &editBuf : nullptr);
        if (!buf) {
            if (ev.key == kKeyEscape && (learnMode || popup.open)) { popup.open = false; setLearn(false); return true; }
            return false;
        }
        if (ev.key == kKeyEnter) {
            if (saving) {
                if (saveUserPreset(saveName, val)) { userPresets = listUserPresets(); curUser = sanitizeName(saveName); curFactory = -1; std::copy(val, val + kParamCount, userVals); saving = false; saveError = false; }
                else saveError = true;
            } else endEdit(true);
            repaint(); return true;
        }
        if (ev.key == kKeyEscape) { if (saving) saving = false; else endEdit(false); repaint(); return true; }
        if (ev.key == kKeyBackspace || ev.key == kKeyDelete) { if (!buf->empty()) buf->pop_back(); repaint(); return true; }
        if (ev.key >= 32 && ev.key < 127 && buf->size() < (saving ? 40u : 12u)) { *buf += (char)ev.key; repaint(); return true; }
        return true;
    }

    bool onMouse(const MouseEvent& ev) override {
        const float s = uiScale(), x = ev.pos.getX() / s, y = ev.pos.getY() / s;
        if (saving) return true;
        if (ev.press) statusFlash = 0;
        if (ev.button == 2 && ev.press) {
            if (menuOpen) { const int m = menuHit(x, y); if (m >= 0 && menuItem(m).kind == 1) { pendingDelete = menuItem(m).idx; repaint(); } return true; }
            const int i = hit(x, y);
            if (learnMode && i >= 0) { const uint32_t p = controls()[i].param; char b[24]; snprintf(b, sizeof b, "unmap:%u", p); setState("cmd", b); paramCC[p] = -1; repaint(); return true; }
            return false;
        }
        if (ev.button != 1) return false;
        if (!ev.press) {
            if (dragSrc > 0) {                     // drop a modulation source
                const int i = hit(x, y);
                if (i >= 0) { const int d = destForParam(effParam(controls()[i])); if (d >= 0) assign(dragSrc, d); }
                dragSrc = -1; repaint(); return true;
            }
            if (active >= 0) { editParameter(effParam(controls()[active]), false); active = -1; repaint(); return true; }
            return false;
        }
        if (editing >= 0) endEdit(true);
        if (popup.open) {
            const int it = popupHit(x, y);
            if (it >= 0) commit(popup.param, (float)it);
            popup.open = false; repaint(); return true;
        }
        if (menuOpen) {
            const int m = menuHit(x, y);
            if (m >= 0) {
                const MRect r = menuItem(m);
                if (r.kind == 2) { menuOpen = false; saving = true; saveError = false; saveName = curUser; }
                else if (r.kind == 1 && pendingDelete == r.idx) { deleteUserPreset(userPresets[r.idx]); if (curUser == userPresets[r.idx]) curUser.clear(); userPresets = listUserPresets(); pendingDelete = -1; repaint(); return true; }
                else { menuOpen = false; if (r.kind == 0) applyFactory(r.idx); else applyUser(userPresets[r.idx]); }
            } else menuOpen = false;
            pendingDelete = -1; menuHover = -1; repaint(); return true;
        }
        const int hh = headerHit(x, y);
        if (hh == 1) { setLearn(!learnMode); return true; }
        if (hh == 2) { commit(kMidiOn, val[kMidiOn] > 0.5f ? 0.f : 1.f); return true; }
        const int pb = presetBarHit(x, y);
        if (pb == 0) { step(-1); return true; }
        if (pb == 2) { step(1); return true; }
        if (pb == 1) { userPresets = listUserPresets(); menuOpen = true; menuHover = -1; pendingDelete = -1; repaint(); return true; }
        if (!learnMode && inRect(x, y, TAP_X, TAP_Y, TAP_W, TAP_H)) { tap(ev.time); return true; }
        if (!learnMode && inRect(x, y, CLR2_X, CLR2_Y, CLR2_W, CLR2_H)) { setState("cmd", "panic"); flash("Echoes cleared"); return true; }
        if (!learnMode) { const int ch = chipHit(x, y); if (ch > 0) { dragSrc = ch; dragPX = x; dragPY = y; repaint(); return true; } }
        if (learnMode && clearHit(x, y)) { setState("cmd", "clear"); for (int& m : paramCC) m = -1; repaint(); return true; }

        int seg = 0; const int i = hit(x, y, &seg);
        if (i < 0) { selSlot = -1; return false; }
        const Ctl& c = controls()[i];
        const uint32_t p = effParam(c);
        if (learnMode) { learnWaiting = (int)c.param; char b[24]; snprintf(b, sizeof b, "learn:%u", c.param); setState("cmd", b); repaint(); return true; }
        switch (c.type) {
        case Toggle: commit(p, val[p] > 0.5f ? 0.f : 1.f); return true;
        case Segment: commit(p, (float)seg); return true;
        case Stepper: commit(p, (float)(((int)(val[p] + .5f) + seg + kNumStyles) % kNumStyles)); return true;
        case SlotSrc: case SlotDst: popup.open = true; popup.param = p; popup.x = c.x - 20; popup.y = c.y + 12; popup.hover = -1;
            selSlot = (int)(p - kSlotFirst) / 4; repaint(); return true;
        case SlotByp: commit(p, val[p] > 0.5f ? 0.f : 1.f); return true;
        case SlotDel: clearSlot((int)(p - kSlotFirst) / 4); return true;
        default: break;
        }
        if (ev.mod & (kModifierControl | kModifierSuper)) { commit(p, defv[p]); return true; }
        if (i == lastClick && ev.time - lastClickTime < 350) {
            lastClick = -1; editing = i;
            char b[48]; fmt(p, b, sizeof b); editBuf = b;
            const size_t sp = editBuf.find(' '); if (sp != std::string::npos && !isPan(p)) editBuf.resize(sp);
            if (!editBuf.empty() && editBuf.back() == '%') editBuf.pop_back();
            if ((p == kF1Freq || p == kF2Freq) && val[p] >= 1000) { char kb[16]; snprintf(kb, sizeof kb, "%.2fk", val[p] / 1000); editBuf = kb; }
            if (isPan(p)) { char pb2[16]; snprintf(pb2, sizeof pb2, "%s%.0f", val[p] < 0 ? "L" : "R", std::fabs(val[p]) * 100); editBuf = std::fabs(val[p]) < .005f ? "C" : pb2; }
            if (p == kTimeL || p == kTimeR) { char tb[16]; snprintf(tb, sizeof tb, "%.1f", val[p]); editBuf = tb; }
            if (isTimeParam(p) && val[p] < 1) { char tb[16]; snprintf(tb, sizeof tb, "%.0fms", val[p] * 1000); editBuf = tb; }
            repaint(); return true;
        }
        lastClick = i; lastClickTime = ev.time;
        if (c.type == Mini) selSlot = (int)(p - kSlotFirst) / 4;
        active = i; dragY = y; dragNorm = toNorm(p, val[p]);
        editParameter(p, true); repaint();
        return true;
    }

    bool onMotion(const MotionEvent& ev) override {
        const float s = uiScale(), x = ev.pos.getX() / s, y = ev.pos.getY() / s;
        if (saving) return true;
        if (dragSrc > 0) { dragPX = x; dragPY = y; const int h = hit(x, y); hover = h; repaint(); return true; }
        if (active >= 0) {
            const uint32_t p = effParam(controls()[active]);
            float sens = (ev.mod & kModifierShift) ? 1500.f : 220.f;
            if (paramDef(p).hints & kHintInteger) sens = std::max(60.f, 14.f * (paramDef(p).max - paramDef(p).min));
            if (controls()[active].type == Mini) sens *= (ev.mod & kModifierShift) ? 1.f : 2.f;     // slot amounts: finer by default
            dragNorm = clampf(dragNorm + (dragY - y) / sens, 0.f, 1.f); dragY = y;
            setValue(p, fromNorm(p, dragNorm));
            return true;
        }
        if (popup.open) { const int h = popupHit(x, y); if (h != popup.hover) { popup.hover = h; repaint(); } return true; }
        if (menuOpen) { const int m = menuHit(x, y); if (m != menuHover) { menuHover = m; repaint(); } return true; }
        const int hh = headerHit(x, y); if (hh != hoverHeader) { hoverHeader = hh; repaint(); }
        const int pb = presetBarHit(x, y); if (pb != presetHover) { presetHover = pb; repaint(); }
        const int ch = chipHit(x, y); if (ch != hoverChip) { hoverChip = ch; repaint(); }
        const bool hc = clearHit(x, y); if (hc != hoverClear) { hoverClear = hc; repaint(); }
        const bool ht = inRect(x, y, TAP_X, TAP_Y, TAP_W, TAP_H); if (ht != hoverTap) { hoverTap = ht; repaint(); }
        const bool h2 = inRect(x, y, CLR2_X, CLR2_Y, CLR2_W, CLR2_H); if (h2 != hoverClr2) { hoverClr2 = h2; repaint(); }
        const int h = hit(x, y); if (h != hover) { hover = h; repaint(); }
        return false;
    }

    bool onScroll(const ScrollEvent& ev) override {
        const float s = uiScale(), sx = ev.pos.getX() / s, sy = ev.pos.getY() / s;
        if (menuOpen || saving || popup.open) return true;
        if (presetBarHit(sx, sy) >= 0) { step(ev.delta.getY() > 0 ? -1 : 1); return true; }
        const int i = hit(sx, sy); if (i < 0) return false;
        const Ctl& c = controls()[i]; const uint32_t p = effParam(c);
        if (c.type == Stepper) { commit(p, (float)(((int)(val[p] + .5f) + (ev.delta.getY() > 0 ? 1 : -1) + kNumStyles) % kNumStyles)); return true; }
        if (c.type != Knob && c.type != Mini) return false;
        float st = (ev.mod & kModifierShift) ? 0.004f : (c.type == Mini ? 0.005f : 0.025f);
        if (paramDef(p).hints & kHintInteger) st = 1.f / (paramDef(p).max - paramDef(p).min);
        commit(p, fromNorm(p, toNorm(p, val[p]) + (ev.delta.getY() > 0 ? st : -st)));
        return true;
    }

private:
    float val[kParamCount], defv[kParamCount], userVals[kParamCount] = {};
    int paramCC[kParamCount];
    int active = -1, hover = -1, lastClick = -1, editing = -1, selSlot = -1;
    uint lastClickTime = 0;
    float dragY = 0, dragNorm = 0;
    int dragSrc = -1, hoverChip = -1; float dragPX = 0, dragPY = 0;
    std::string editBuf, statusText; int statusFlash = 0;
    int curFactory = 0, presetHover = -1, menuHover = -1, pendingDelete = -1, hoverHeader = 0;
    std::string curUser; std::vector<std::string> userPresets;
    bool menuOpen = false, saving = false, saveError = false; std::string saveName;
    bool learnMode = false, hoverClear = false, hoverTap = false, hoverClr2 = false; int learnWaiting = -1; float lastLearnEcho = 0;
    double taps[5] = {}; int nTaps = 0, tapFlash = 0;
    ResizeHandle resizeHandle;

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(StrataUI)
};

UI* createUI() { return new StrataUI(); }

END_NAMESPACE_DISTRHO
