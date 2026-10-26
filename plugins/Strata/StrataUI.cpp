// Strata — custom vector UI (DPF/NanoVG). Original design. License: MIT.
#include "DistrhoUI.hpp"
#include "StrataPlugin.hpp"
#include "UserPresets.hpp"
#include "ComponentPresets.hpp"
#include "TapPatterns.hpp"
#include "Help.hpp"
#include "ResizeHandle.hpp"
#include <cctype>
#include <chrono>
#include <cmath>
#include <complex>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#if defined(_WIN32)
extern "C" __declspec(dllimport) void* __stdcall ShellExecuteA(void*, const char*, const char*, const char*, const char*, int);
#endif

START_NAMESPACE_DISTRHO

static void openURL(const char* url) {
#if defined(_WIN32)
    ShellExecuteA(nullptr, "open", url, nullptr, nullptr, 1);
#elif defined(__APPLE__)
    const std::string c = std::string("open '") + url + "' &"; if (std::system(c.c_str())) {}
#else
    const std::string c = std::string("xdg-open '") + url + "' >/dev/null 2>&1 &"; if (std::system(c.c_str())) {}
#endif
}
static bool gTipsOn = true;
static double nowSec() { return std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count(); }

using namespace strata;
using DGL_NAMESPACE::ResizeHandle;
typedef std::complex<float> cplx;

static constexpr float W = 1240.f, H = 850.f;

#define C_BG        Color(0x11, 0x10, 0x0e)
#define C_PANEL     Color(0x1b, 0x19, 0x17)
#define C_PANEL_HI  Color(0x26, 0x23, 0x20)
#define C_PANEL_LO  Color(0x1f, 0x1d, 0x1a)
#define C_LINE      Color(0x33, 0x2f, 0x2a)
#define C_TEXT      Color(0xee, 0xe6, 0xda)
#define C_DIM       Color(0x8e, 0x86, 0x7c)
#define C_FAINT     Color(0x52, 0x4c, 0x45)
#define C_ACC       Color(0xe8, 0xa9, 0x5c)       // sandstone amber
#define C_LEFT      Color(0xe8, 0xa9, 0x5c)
#define C_RIGHT     Color(0x8c, 0xbc, 0xe6)
#define C_DISPLAY   Color(0x0c, 0x0b, 0x0a)
#define C_FREEZE    Color(0x9c, 0xd8, 0xf0)
#define C_FAV       Color(0xff, 0xc8, 0x5a)       // favourite star

static Color withAlpha(const Color& c, float a) { Color r(c); r.alpha = a; return r; }
static Color filterColor(int f) {
    static const Color c[kNumFilters] = { Color(0xe8, 0xa9, 0x5c), Color(0x8c, 0xbc, 0xe6), Color(0x5f, 0xd0, 0xb4), Color(0xd6, 0x8c, 0xd0), Color(0xd8, 0xd0, 0x6a), Color(0xf0, 0x80, 0x68) };
    return c[f % kNumFilters];
}
// chips: v2 sources use their em::Source id (1..17); the classic (2.x) LFO 1, LFO 2 and envelope use 100..102
static constexpr int kClassicChip = 100;
static Color srcColor(int s) {
    if (s >= kClassicChip) return Color(0xd8, 0xc8, 0xa8);
    if (s >= em::kSrcXlfo1 && s <= em::kSrcXlfo4) return Color(0x5f, 0xd0, 0xb4);
    if (s == em::kSrcEg1 || s == em::kSrcEg2) return Color(0xff, 0x86, 0x52);
    if (s == em::kSrcEf1 || s == em::kSrcEf2) return Color(0xc8, 0xd6, 0x5a);
    if (s >= em::kSrcXy1X && s <= em::kSrcXy2Y) return Color(0x6f, 0xa8, 0xf0);
    if (s >= em::kSrcVelocity) return Color(0xc0, 0x9c, 0xff);
    return C_DIM;
}
static const char* chipShort(int s) {
    if (s >= kClassicChip) { static const char* const n[3] = { "LFO 1", "LFO 2", "ENV" }; return n[(s - kClassicChip) % 3]; }
    return em::sourceShort(s);
}
static const char* chipName(int s) {
    if (s >= kClassicChip) { static const char* const n[3] = { "Classic LFO 1", "Classic LFO 2", "Classic Envelope" }; return n[(s - kClassicChip) % 3]; }
    return em::sourceName(s);
}
static const char* classicSrcShort(int s) {
    static const char* n[kNumSources] = { "-", "LFO 1", "LFO 2", "ENV", "VEL", "AT", "KEY", "BEND", "WHEEL" };
    return n[s < 0 || s >= kNumSources ? 0 : s];
}

// ---------------------------------------------------------------- layout
static constexpr float R1 = 64, R1H = 250, R2 = 322, R2H = 200, R3 = 530, R3H = 296;
static constexpr float LX = 16, EX = 314, RX = 934, SIDEW = 290, ECHOW = 612;
static constexpr float DISP_X = EX + 14, DISP_Y = R1 + 34, DISP_W = ECHOW - 28, DISP_H = 150;
static constexpr float VIEW_X = EX + ECHOW - 14 - 120, VIEW_Y = R1 + 19, VIEW_W = 120;          // MAP / TAPS
static constexpr float PAT_X = VIEW_X - 104, PAT_Y = R1 + 9, PAT_W = 96, PAT_H = 20;           // tap patterns
static constexpr float FRZ_X = EX + 440, FRZ_Y = R1 + 205, FRZ_W = 64, FRZ_H = 26;
static constexpr float TAP_X = EX + 512, TAP_Y = R1 + 205, TAP_W = 86, TAP_H = 26;
static constexpr float FX0 = 16, FW0 = 684, EFX = 708, EFW = 516;                          // filters, effects
static constexpr float FT_X = 30, FT_Y = R2 + 12, FT_W = 54, FT_H = 26;
static constexpr float STYLE_X = 392, STYLE_Y = R2 + 12, STYLE_W = 150;
static constexpr float FD_X = 30, FD_Y = R2 + 84, FD_W = 312, FD_H = 104;
static constexpr float MX = 16, MY = R3, MW = 580, MH = R3H;                              // modulation sources
static constexpr float SX = 604, SY = R3, SW = 340, SH = R3H;                             // modulation list
static constexpr float OX = 952, OY = R3, OW = 272, OH = R3H;                             // output
static constexpr float CLR2_X = OX + 130, CLR2_Y = OY + 258, CLR2_W = 128, CLR2_H = 24;
static constexpr float SEG_H = 20, CHIP_W = 62, CHIP_H = 18;
static constexpr float TAB_Y = MY + 18, TAB_W = 44, TAB_H = 20;
static constexpr int kNumTabs = 12, kTabClassic = 11;          // XLFO 1-4, EG 1-2, EF 1-2, XY 1-2, MIDI, CLASSIC
static constexpr float SLOT_ROWH = 22, SLOT_COLW = 320, SLOT_Y0 = SY + 34; static constexpr int SLOT_ROWS = 11;
static constexpr float STEP_X = MX + 16, STEP_Y = MY + 108, STEP_W = 380, STEP_H = 168;
static constexpr float XY_X = MX + 20, XY_Y = MY + 44, XY_W = 300, XY_H = 236;
static constexpr float COMP_W = 110, COMP_H = 20, COMP_X = MX + MW - 14 - COMP_W, COMP_Y = MY + 78;
static constexpr float CL_COLW = 188;
static constexpr float STATUS_Y = 838;

enum WType { Knob, Toggle, Segment, Stepper };
enum Group { GGlobal, GFilt, GXlfo, GEg, GEf, GClassic };
struct Ctl { WType type; int group, field; float x, y, r; const char* label; };   // Segment/Stepper: r = width

static float effCol(int i) { return EFX + 54 + i * 102.f; }

static std::vector<Ctl> buildControls() {
    std::vector<Ctl> c;
    // left / right sides
    for (int s = 0; s < 2; ++s) {
        const float x = s == 0 ? LX : RX; const int o = s;
        c.push_back({ Segment, GGlobal, (int)kSyncL + o, x + 145, R1 + 56, 252, "" });
        c.push_back({ Knob, GGlobal, (int)kTimeL + o, x + 62, R1 + 128, 30, "TIME" });
        c.push_back({ Knob, GGlobal, (int)kOffL + o, x + 160, R1 + 128, 17, "OFFSET" });
        c.push_back({ Knob, GGlobal, (int)kPanL + o, x + 238, R1 + 128, 17, "PAN" });
        c.push_back({ Knob, GGlobal, (int)kFbL + o, x + 62, R1 + 210, 20, "FEEDBACK" });
        c.push_back({ Knob, GGlobal, s == 0 ? (int)kXfLR : (int)kXfRL, x + 160, R1 + 210, 17, s == 0 ? "CROSS > R" : "CROSS > L" });
        c.push_back({ Toggle, GGlobal, (int)kInvL + o, x + 222, R1 + 210, 0, "INVERT" });
    }
    c.push_back({ Toggle, GGlobal, kLink, RX + 222, R1 + 20, 0, "LINK" });
    // echoes panel
    c.push_back({ Segment, GGlobal, kMode, EX + 80, R1 + 218, 132, "" });
    c.push_back({ Knob, GGlobal, kGlide, EX + 186, R1 + 214, 15, "SMOOTH" });
    c.push_back({ Segment, GGlobal, kInput, EX + 330, R1 + 218, 196, "" });
    // filters (field: of the selected filter)
    c.push_back({ Segment, GFilt, kFType, FT_X + 161, R2 + 64, 322, "" });
    c.push_back({ Segment, GFilt, kFSlope, FT_X + 434, R2 + 64, 200, "" });
    c.push_back({ Knob, GFilt, kFFreq, 404, R2 + 120, 24, "FREQ" });
    c.push_back({ Knob, GFilt, kFPeak, 490, R2 + 120, 18, "PEAK" });
    c.push_back({ Knob, GFilt, kFGain, 564, R2 + 120, 18, "GAIN" });
    c.push_back({ Knob, GFilt, kFPan, 638, R2 + 120, 18, "PAN" });
    c.push_back({ Segment, GGlobal, kRouting, 543, R2 + 184, 276, "" });
    // effects
    c.push_back({ Knob, GGlobal, kDrive, effCol(0), R2 + 102, 22, "DRIVE" });
    c.push_back({ Toggle, GGlobal, kLofiOn, effCol(1) - 24, R2 + 40, 0, "LO-FI" });
    c.push_back({ Knob, GGlobal, kLofi, effCol(1), R2 + 102, 22, "AMOUNT" });
    c.push_back({ Toggle, GGlobal, kDiffOn, effCol(2) - 24, R2 + 40, 0, "DIFFUSE" });
    c.push_back({ Knob, GGlobal, kDiffuse, effCol(2), R2 + 102, 22, "AMOUNT" });
    c.push_back({ Toggle, GGlobal, kDynOn, effCol(3) - 24, R2 + 40, 0, "DYNAMICS" });
    c.push_back({ Knob, GGlobal, kDyn, effCol(3), R2 + 102, 22, "DUCK / SWELL" });
    c.push_back({ Toggle, GGlobal, kPitchOn, effCol(4) - 24, R2 + 40, 0, "PITCH" });
    c.push_back({ Knob, GGlobal, kPitch, effCol(4), R2 + 96, 20, "SHIFT" });
    c.push_back({ Toggle, GGlobal, kPitchMirror, effCol(4) - 30, R2 + 164, 0, "MIRROR" });
    c.push_back({ Toggle, GGlobal, kPitchInLoop, effCol(4) - 30, R2 + 186, 0, "IN LOOP" });
    // output
    c.push_back({ Segment, GGlobal, kChanMode, OX + 136, OY + 46, 220, "" });
    c.push_back({ Knob, GGlobal, kWidth, OX + 50, OY + 108, 17, "WIDTH" });
    c.push_back({ Knob, GGlobal, kWetLevel, OX + 136, OY + 108, 17, "WET" });
    c.push_back({ Knob, GGlobal, kWetPan, OX + 222, OY + 108, 17, "WET PAN" });
    c.push_back({ Knob, GGlobal, kInGain, OX + 50, OY + 196, 18, "INPUT" });
    c.push_back({ Knob, GGlobal, kMix, OX + 136, OY + 192, 26, "MIX" });
    c.push_back({ Knob, GGlobal, kOutGain, OX + 222, OY + 196, 18, "OUTPUT" });
    c.push_back({ Toggle, GGlobal, kLockMix, OX + 30, OY + 270, 0, "LOCK MIX" });
    // XLFO editor
    c.push_back({ Segment, GXlfo, em::kLMode, MX + 82, MY + 62, 130, "" });
    c.push_back({ Segment, GXlfo, em::kLSync, MX + 296, MY + 62, 250, "" });
    c.push_back({ Segment, GXlfo, em::kLWave, MX + 206, MY + 88, 380, "" });
    c.push_back({ Knob, GXlfo, em::kLRate, MX + 452, MY + 150, 20, "RATE" });
    c.push_back({ Stepper, GXlfo, em::kLDiv, MX + 452, MY + 150, 74, "" });
    c.push_back({ Knob, GXlfo, em::kLBalance, MX + 530, MY + 150, 20, "BALANCE" });
    c.push_back({ Knob, GXlfo, em::kLGlide, MX + 530, MY + 150, 20, "GLIDE" });
    c.push_back({ Stepper, GXlfo, em::kLSteps, MX + 452, MY + 222, 74, "" });
    // EG editor
    c.push_back({ Segment, GEg, em::kETrigger, MX + 116, MY + 62, 170, "" });
    c.push_back({ Knob, GEg, em::kEAttack, MX + 60, MY + 150, 22, "ATTACK" });
    c.push_back({ Knob, GEg, em::kEDecay, MX + 140, MY + 150, 22, "DECAY" });
    c.push_back({ Knob, GEg, em::kESustain, MX + 220, MY + 150, 22, "SUSTAIN" });
    c.push_back({ Knob, GEg, em::kERelease, MX + 300, MY + 150, 22, "RELEASE" });
    c.push_back({ Knob, GEg, em::kEThreshold, MX + 400, MY + 150, 22, "THRESHOLD" });
    // EF editor
    c.push_back({ Segment, GEf, em::kFSource, MX + 126, MY + 62, 190, "" });
    c.push_back({ Knob, GEf, em::kFAttack, MX + 60, MY + 150, 22, "ATTACK" });
    c.push_back({ Knob, GEf, em::kFRelease, MX + 140, MY + 150, 22, "RELEASE" });
    c.push_back({ Knob, GEf, em::kFGain, MX + 220, MY + 150, 22, "GAIN" });
    // classic (2.x) sources (field: parameter)
    for (int l = 0; l < 2; ++l) {
        const float x0 = MX + 14 + l * CL_COLW; const uint32_t b = kL1Wave + 6 * l;
        c.push_back({ Segment, GClassic, (int)b, x0 + 88, MY + 80, 172, "" });
        c.push_back({ Segment, GClassic, (int)(b + 2), x0 + 88, MY + 106, 172, "" });
        c.push_back({ Knob, GClassic, (int)(b + 3), x0 + 46, MY + 200, 17, "RATE" });
        c.push_back({ Knob, GClassic, (int)(b + 1), x0 + 128, MY + 200, 15, "BALANCE" });
        c.push_back({ Toggle, GClassic, (int)(b + 5), x0 + 20, MY + 262, 0, "RETRIGGER" });
    }
    { const float x0 = MX + 14 + 2 * CL_COLW;
      c.push_back({ Segment, GClassic, kEgTrigger, x0 + 44, MY + 80, 80, "" });
      static const char* const eg[4] = { "A", "D", "S", "R" };
      for (int i = 0; i < 4; ++i) c.push_back({ Knob, GClassic, (int)(kEgAttack + i), x0 + 18 + i * 36.f, MY + 200, 13, eg[i] });
      c.push_back({ Knob, GClassic, kEgThreshold, x0 + 162, MY + 200, 13, "THR" }); }
    return c;
}
static const std::vector<Ctl>& controls() { static std::vector<Ctl> c = buildControls(); return c; }

static const char* const* segLabels(int group, int field, int& n) {
    static const char* const sync[] = { "FREE", "STRAIGHT", "DOTTED", "TRIPLET" };
    static const char* const csync[] = { "FREE", "STR", "DOT", "TRIP" };
    static const char* const mode[] = { "TAPE", "STRETCH" };
    static const char* const input[] = { "STEREO", "MONO", "P-P L", "P-P R" };
    static const char* const shape[] = { "LP", "BP", "HP", "BELL", "LO SH", "HI SH", "NOTCH" };
    static const char* const slope[] = { "6 DB", "12 DB", "24 DB", "48 DB" };
    static const char* const route[] = { "SERIAL", "PARALLEL", "PER CHANNEL" };
    static const char* const chm[] = { "LEFT / RIGHT", "MID / SIDE" };
    static const char* const lm[] = { "WAVE", "STEPS" };
    static const char* const sy[] = { "FREE", "SYNC", "DOTTED", "TRIPLET" };
    static const char* const wv[] = { "SIN", "TRI", "SAW+", "SAW-", "SQR", "RND", "S&H" };
    static const char* const tr[] = { "MIDI", "AUDIO" };
    static const char* const efs[] = { "INPUT", "SIDECHAIN" };
    static const char* const cw[] = { "TRI", "SIN", "SQR", "RND", "S&H" };
    static const char* const ctr[] = { "AUD", "MIDI" };
    n = 0;
    if (group == GGlobal) {
        switch (field) {
        case kSyncL: case kSyncR: n = 4; return sync;
        case kMode: n = 2; return mode;
        case kInput: n = 4; return input;
        case kRouting: n = 3; return route;
        case kChanMode: n = 2; return chm;
        default: return nullptr;
        }
    }
    if (group == GFilt) { if (field == kFType) { n = 7; return shape; } if (field == kFSlope) { n = 4; return slope; } }
    if (group == GXlfo) { if (field == em::kLMode) { n = 2; return lm; } if (field == em::kLSync) { n = 4; return sy; } if (field == em::kLWave) { n = 7; return wv; } }
    if (group == GEg && field == em::kETrigger) { n = 2; return tr; }
    if (group == GEf && field == em::kFSource) { n = 2; return efs; }
    if (group == GClassic) {
        if (field == kL1Wave || field == kL2Wave) { n = 5; return cw; }
        if (field == kL1Sync || field == kL2Sync) { n = 4; return csync; }
        if (field == kEgTrigger) { n = 2; return ctr; }
    }
    return nullptr;
}

// header:  logo · preset bar · A B COPY · UNDO REDO · LEARN MIDI · HELP
static constexpr float PS_X = 484.f, PS_Y = 20.f, PS_W = 260.f, PS_H = 28.f, PS_ARROW = 30.f;
static constexpr float A_X = 756.f, AB_W = 30.f, COPY_X = 818.f, COPY_W = 46.f;
static constexpr float UNDO_X = 874.f, REDO_X = 926.f, UR_W = 48.f;
static constexpr float LEARN_X = 984.f, LEARN_W = 70.f, MIDI_X = 1060.f, MIDI_W = 50.f, HELP_X = 1116.f, HELP_W = 50.f;
static constexpr float PM_COLW = 120.f, PM_ROW = 22.f, PM_HEAD = 22.f, PM_PAD = 8.f;

static bool isEqShape(int t) { return t == kBell || t == kLowShelf || t == kHighShelf; }
// a filter's response (linear prototype of the engine's filters)
static cplx filterH(const float* v, int f, float hz) {
    const int type = (int)(v[fpar(f, kFType)] + .5f);
    int stages = slopeStages((int)(v[fpar(f, kFSlope)] + .5f));
    const float fc = v[fpar(f, kFFreq)], r = std::pow(clampf(v[fpar(f, kFPeak)], 0.f, 1.f), 0.6f);
    const StyleDef& sd = styleDef((int)(v[fpar(f, kFStyle)] + .5f));
    const float w = hz / fc, G = dbToGain(v[fpar(f, kFGain)]);
    const cplx s(0, w);
    cplx h(1, 0);
    if (stages == 0 && type != kBandPass && type != kNotch && type != kBell) {
        const cplx lo = 1.f / (s + 1.f), hi = s / (s + 1.f);
        switch (type) { case kLowPass: return lo; case kHighPass: return hi; case kLowShelf: return 1.f + (G - 1.f) * lo; default: return 1.f + (G - 1.f) * hi; }
    }
    const bool wide = stages == 0; if (stages == 0) stages = 1;
    if (type <= kHighPass) {
        const float* kb = bwK(stages);
        for (int i = 0; i < stages; ++i) {
            const float k = std::max(i == stages - 1 ? kb[i] * (1 - r) + sd.kMin * r : kb[i], 0.035f);
            const cplx den = s * s + k * s + 1.f;
            h *= type == kLowPass ? 1.f / den : (type == kHighPass ? s * s / den : s * std::max(k, 0.12f) / den);
        }
        return h;
    }
    const float k = std::max(eqDamping(v[fpar(f, kFPeak)]) * (wide ? 2.f : 1.f), 0.02f), gs = std::pow(G, 1.f / stages);
    for (int i = 0; i < stages; ++i) {
        const cplx den = s * s + k * s + 1.f, bp = k * s / den, lp = 1.f / den, hp = s * s / den;
        switch (type) { case kBell: h *= 1.f + (gs - 1.f) * bp; break; case kLowShelf: h *= 1.f + (gs - 1.f) * lp; break;
                        case kHighShelf: h *= 1.f + (gs - 1.f) * hp; break; default: h *= 1.f - bp; break; }
    }
    return h;
}
// the combined response of the filters in the loop; side: -1 both (serial/parallel), 0 left/odd, 1 right/even (per channel)
static cplx loopH(const float* v, float hz, int side = -1) {
    const int route = (int)(v[kRouting] + .5f);
    cplx acc(route == kParallel ? 0.f : 1.f, 0); int cnt = 0;
    for (int f = 0; f < kNumFilters; ++f) {
        if (v[fpar(f, kFOn)] < 0.5f) continue;
        if (route == kPerChannel && side >= 0 && (f & 1) != side) continue;
        const cplx h = filterH(v, f, hz);
        if (route == kParallel) acc += h; else acc *= h;
        ++cnt;
    }
    if (!cnt) return cplx(1, 0);
    return route == kParallel ? acc * (cnt == 2 ? 0.7071f : 1.f / std::sqrt((float)cnt)) : acc;
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
        std::copy(val, val + kParamCount, before);
        std::fill(liveOff, liveOff + kNumDests2, 0.f); std::fill(liveSrc, liveSrc + em::kNumSources, 0.f); std::fill(liveClassic, liveClassic + kNumSources, 0.f);
        for (int f = 0; f < kNumFilters; ++f) liveHz[f] = val[fpar(f, kFFreq)];
    }

protected:
    float uiScale() const { return (float)getWidth() / W; }

    // ---- values ---------------------------------------------------------------
    void setValue(uint32_t p, float v) {
        const ParamDef& d = paramDef(p); v = clampf(v, d.min, d.max);
        if (d.hints & kHintInteger) v = std::round(v);
        if (v == val[p]) return;
        val[p] = v; setParameterValue(p, v); repaint();
    }
    void commit(uint32_t p, float v) { editParameter(p, true); setValue(p, v); editParameter(p, false); }
    void parameterChanged(uint32_t i, float v) override {
        if (i >= kParamCount) return;
        if (i == kOutLearn) {
            if (v != lastLearnEcho && v > 0) {
                const int code = (int)v % 1000000 - 1, prm = code / 128, cc = code % 128;
                for (int& m : paramCC) if (m == cc) m = -1;
                if (prm >= 0 && prm < (int)kParamCount) paramCC[prm] = cc;
                learnWaiting = -1;
            }
            lastLearnEcho = v; repaint(); return;
        }
        if (val[i] != v) { val[i] = v; if (!gesture && isSetting(i)) before[i] = v; repaint(); }
    }
    void stateChanged(const char* key, const char* value) override {
        if (std::strcmp(key, "midimap") != 0) return;
        for (int& m : paramCC) m = -1;
        const char* q = value;
        while (q && *q) { int cc = -1, prm = -1; if (std::sscanf(q, "%d:%d", &cc, &prm) == 2 && prm >= 0 && prm < (int)kParamCount) paramCC[prm] = cc;
            const char* s = std::strchr(q, ';'); if (!s) break; q = s + 1; }
        repaint();
    }

    // ---- undo / redo / A-B (outputs are not part of a setting) --------------------
    typedef std::vector<float> Snap;
    Snap snapshot() const { Snap s(val, val + kParamCount); s.push_back((float)abSlot); return s; }
    static bool differs(const float* a, const float* b) { for (uint32_t i = 0; i < kParamCount; ++i) if (isSetting(i) && a[i] != b[i]) return true; return false; }
    void beginGesture() { gesture = true; std::copy(val, val + kParamCount, before); }
    void endGesture() {
        gesture = false;
        if (differs(before, val)) { Snap u(before, before + kParamCount); u.push_back((float)abSlot); undoStack.push_back(u); if (undoStack.size() > 200) undoStack.erase(undoStack.begin()); redoStack.clear(); }
        std::copy(val, val + kParamCount, before); repaint();
    }
    void applyAll(const float* s) { for (uint32_t i = 0; i < kParamCount; ++i) if (isSetting(i) && val[i] != s[i]) commit(i, s[i]); }
    void restoreSnap(const Snap& s) {
        if (s.size() > kParamCount && (int)s[kParamCount] != abSlot) { abStore[abSlot] = snapshot(); abSlot = (int)s[kParamCount]; }
        applyAll(s.data()); std::copy(val, val + kParamCount, before);
    }
    void undo() { if (undoStack.empty()) return; redoStack.push_back(snapshot()); restoreSnap(undoStack.back()); undoStack.pop_back(); flash("Undo"); }
    void redo() { if (redoStack.empty()) return; undoStack.push_back(snapshot()); restoreSnap(redoStack.back()); redoStack.pop_back(); flash("Redo"); }
    void switchAB(int to) {
        if (to == abSlot) return;
        abStore[abSlot] = snapshot(); if (abStore[to].empty()) abStore[to] = snapshot();
        beginGesture(); applyAll(abStore[to].data()); endGesture();
        abSlot = to; flash(to == 0 ? "Setting A" : "Setting B");
    }
    void copyAB() { abStore[1 - abSlot] = snapshot(); flash(abSlot == 0 ? "Copied A to B" : "Copied B to A"); }

    // ---- parameter of a control -------------------------------------------------------
    bool linked() const { return val[kLink] > 0.5f; }
    static int clampTab(int t, int n) { return t < 0 ? 0 : (t >= n ? n - 1 : t); }
    uint32_t effParam(const Ctl& c) const {
        switch (c.group) {
        case GFilt: return fpar(selF, c.field);
        case GXlfo: return lp_(std::min(tab, em::kNumXlfo - 1), c.field);
        case GEg: return ep(clampTab(tab - 4, em::kNumEg), c.field);
        case GEf: return fp(clampTab(tab - 6, em::kNumEf), c.field);
        case GClassic: {
            const uint32_t p = (uint32_t)c.field;
            for (int l = 0; l < 2; ++l) { const uint32_t b = kL1Wave + 6 * l; if (p == b + 3 && val[b + 2] > 0.5f) return b + 4; }
            return p; }
        default: {
            const uint32_t p = (uint32_t)c.field;
            if (p == kTimeL && val[kSyncL] > 0.5f) return kDivL;
            if (p == kTimeR && val[kSyncR] > 0.5f) return kDivR;
            return p; }
        }
    }
    bool visible(const Ctl& c) const {
        if (c.group == GXlfo) {
            if (tab > 3) return false;
            const int l = tab; const bool steps = val[lp_(l, em::kLMode)] > 0.5f, sync = val[lp_(l, em::kLSync)] > 0.5f;
            if (c.field == em::kLWave || c.field == em::kLBalance) return !steps;
            if (c.field == em::kLGlide || c.field == em::kLSteps) return steps;
            if (c.field == em::kLRate) return !sync;
            if (c.field == em::kLDiv) return sync;
            return true;
        }
        if (c.group == GEg) return tab == 4 || tab == 5;
        if (c.group == GEf) return tab == 6 || tab == 7;
        if (c.group == GClassic) return tab == kTabClassic;
        return true;
    }
    bool fOn(int f) const { return val[fpar(f, kFOn)] > 0.5f; }
    int filtersOn() const { int n = 0; for (int f = 0; f < kNumFilters; ++f) n += fOn(f); return n; }
    bool dimmed(uint32_t p) const {
        if (linked() && (p == kSyncR || p == kTimeR || p == kDivR || p == kOffR)) return true;
        if ((p == kOffL && val[kSyncL] < 0.5f) || (p == kOffR && val[kSyncR] < 0.5f)) return true;
        for (int f = 0; f < kNumFilters; ++f) {
            if (p == fpar(f, kFGain)) return !fOn(f) || !isEqShape((int)std::round(val[fpar(f, kFType)]));
            for (int k = 1; k < kFBaseFields; ++k) if (p == fpar(f, k)) return !fOn(f);
        }
        if (p == kRouting) return filtersOn() < 2;
        if (p == kEgThreshold) return val[kEgTrigger] > 0.5f;
        if (p >= kEgFirst && p < kEfFirst && (p - kEgFirst) % kEgFields == em::kEThreshold) return val[p - em::kEThreshold + em::kETrigger] < 0.5f;
        if (p == kLofi) return val[kLofiOn] < 0.5f;
        if (p == kDiffuse) return val[kDiffOn] < 0.5f;
        if (p == kDyn) return val[kDynOn] < 0.5f;
        if (p == kPitch || p == kPitchMirror || p == kPitchInLoop) return val[kPitchOn] < 0.5f;
        if (p == kMainLevel || p == kMainPan) return val[kMainOn] < 0.5f;
        return false;
    }

    // ---- formatting ---------------------------------------------------------
    static void fmtMs(float ms, char* b, size_t n) {
        if (ms < 10) snprintf(b, n, "%.2f ms", ms); else if (ms < 100) snprintf(b, n, "%.1f ms", ms);
        else if (ms < 1000) snprintf(b, n, "%.0f ms", ms); else snprintf(b, n, "%.2f s", ms / 1000);
    }
    static bool isPan(uint32_t p) {
        if (p == kPanL || p == kPanR || p == kWetPan || p == kMainPan) return true;
        for (int f = 0; f < kNumFilters; ++f) if (p == fpar(f, kFPan)) return true;
        for (int t = 0; t < kNumTaps; ++t) if (p == tpar(t, kTPan)) return true;
        return false;
    }
    static bool isTapTime(uint32_t p) { return p >= kTapFirst && p < kFGainFirst && (p - kTapFirst) % kTapFields == kTTime; }
    void fmtValue(uint32_t p, float v, char* b, size_t n) const {
        int ne = 0;
        if (p == kDivL || p == kDivR) {
            const char* const* l = enumLabels(p, ne); const int sy = (int)(val[p == kDivL ? kSyncL : kSyncR] + .5f);
            snprintf(b, n, "%s%s", l[std::max(0, std::min(ne - 1, (int)std::round(v)))], sy == 2 ? " dot" : (sy == 3 ? " trip" : "")); return;
        }
        if (const char* const* l = enumLabels(p, ne)) { snprintf(b, n, "%s", l[std::max(0, std::min(ne - 1, (int)std::round(v)))]); return; }
        const ParamDef& d = paramDef(p);
        if (d.hints & kHintBoolean) { snprintf(b, n, "%s", v > 0.5f ? "On" : "Off"); return; }
        if (p == kTimeL || p == kTimeR) { fmtMs(v, b, n); return; }
        if (isPan(p)) { if (std::fabs(v) < .005f) snprintf(b, n, "Center"); else snprintf(b, n, "%s %.0f%%", v < 0 ? "L" : "R", std::fabs(v) * 100); return; }
        if (p == kOffL || p == kOffR) { snprintf(b, n, "%+.1f%%", v); return; }
        if (p == kPitch) { snprintf(b, n, "%+.0f st", v); return; }
        if (isTapTime(p)) { snprintf(b, n, "%.1f%%", v * 100); return; }
        if (isSlotAmount(p)) { const float a = std::fabs(v) * 100; snprintf(b, n, a < 1 && a > 0 ? "%+.2f%%" : "%+.1f%%", v * 100); return; }
        if (std::strcmp(d.unit, "Hz") == 0) { if (v >= 1000) snprintf(b, n, "%.2f kHz", v / 1000); else snprintf(b, n, v < 10 ? "%.2f Hz" : "%.0f Hz", v); return; }
        if (std::strcmp(d.unit, "dB") == 0) { snprintf(b, n, "%+.1f dB", v); return; }
        if (std::strcmp(d.unit, "s") == 0 || isTimeParam(p)) { if (v < 1) snprintf(b, n, "%.0f ms", v * 1000); else snprintf(b, n, "%.2f s", v); return; }
        if (p >= kXlfoFirst && p < kEgFirst && (p - kXlfoFirst) % kXlfoFields == em::kLSteps) { snprintf(b, n, "%.0f", v); return; }
        if (d.hints & kHintBipolar) { snprintf(b, n, "%+.0f%%", v * 100); return; }
        snprintf(b, n, "%.0f%%", v * 100);
    }
    void fmt(uint32_t p, char* b, size_t n) const { fmtValue(p, val[p], b, n); }
    bool parse(uint32_t p, const std::string& in, float& out) const {
        std::string s; for (char ch : in) if (!std::isspace((unsigned char)ch)) s += (char)std::tolower((unsigned char)ch);
        if (s.empty()) return false;
        int ne = 0;
        if (const char* const* l = enumLabels(p, ne)) {
            for (int i = 0; i < ne; ++i) { std::string lab; for (const char* c = l[i]; *c; ++c) if (*c != ' ') lab += (char)std::tolower((unsigned char)*c);
                if (lab.compare(0, s.size(), s) == 0) { out = (float)i; return true; } }
            return false;
        }
        if (isPan(p)) {
            if (s[0] == 'c') { out = 0; return true; }
            const float sign = s[0] == 'l' ? -1.f : 1.f;
            out = sign * (float)std::atof((s[0] == 'l' || s[0] == 'r') ? s.c_str() + 1 : s.c_str()) / 100.f; return true;
        }
        char* end = nullptr; double x = std::strtod(s.c_str(), &end);
        if (end == s.c_str()) return false;
        const std::string suf(end); const ParamDef& d = paramDef(p);
        if (suf == "k" || suf == "khz") x *= 1000;
        if (p == kTimeL || p == kTimeR) { out = (float)(suf == "s" ? x * 1000 : x); return true; }
        if (std::strcmp(d.unit, "s") == 0 || isTimeParam(p)) { out = (float)(suf == "ms" ? x / 1000 : x); return true; }
        if (std::strcmp(d.unit, "Hz") == 0 || std::strcmp(d.unit, "dB") == 0 || std::strcmp(d.unit, "st") == 0 || p == kOffL || p == kOffR) { out = (float)x; return true; }
        if (p >= kXlfoFirst && p < kEgFirst && (p - kXlfoFirst) % kXlfoFields == em::kLSteps) { out = (float)x; return true; }
        out = (float)(x / 100.0); return true;
    }

    // ---- drawing --------------------------------------------------------------
    void onNanoDisplay() override {
        const float s = uiScale();
        save(); scale(s, s);
        beginPath(); rect(0, 0, W, H); fillColor(C_BG); fill();
        fontFace(NANOVG_DEJAVU_SANS_TTF);
        drawHeader();
        drawSides();
        drawEchoPanel();
        drawFilterPanel();
        drawEffectsPanel();
        drawSources();
        drawSlots();
        drawOutputPanel();
        const std::vector<Ctl>& cs = controls();
        for (int i = 0; i < (int)cs.size(); ++i) if (visible(cs[i])) drawCtl(cs[i], i == active || i == hover);
        drawStatus();
        if (editing >= 0) drawEditBox();
        if (dragSrc > 0) drawChip(dragSrc, dragPX - CHIP_W / 2, dragPY - CHIP_H / 2, true, true);
        if (kpan.open) drawKnobPanel();
        if (popup.open) drawPopup();
        if (menuOpen) drawPresetMenu();
        if (saving) drawSaveBox();
        if (helpOpen) drawHelp();
        else if (tipShown && gTipsOn && dragSrc <= 0) drawTooltip();
        restore();
        if (tapFlash > 0) { --tapFlash; repaint(); }
    }

    void panel(float x, float y, float w, float h, const char* title = "") {
        beginPath(); roundedRect(x, y, w, h, 6); fillColor(C_PANEL); fill(); strokeColor(C_LINE); strokeWidth(1); stroke();
        if (title[0]) { fontSize(9.5f); textLetterSpacing(2.2f); fillColor(C_DIM); textAlign(ALIGN_LEFT | ALIGN_TOP); text(x + 14, y + 10, title, nullptr); }
    }
    void caption(float x, float y, const char* t, int align = ALIGN_CENTER) { fontSize(7.5f); textLetterSpacing(1.8f); textAlign(align | ALIGN_MIDDLE); fillColor(C_FAINT); text(x, y, t, nullptr); }
    void displayBox(float x, float y, float w, float h) { beginPath(); roundedRect(x, y, w, h, 4); fillColor(C_DISPLAY); fill(); strokeColor(C_LINE); strokeWidth(1); stroke(); }
    void drawButton(float x, float y, float w, float h, const char* label, bool on, bool hot, const Color& acc = C_ACC) {
        beginPath(); roundedRect(x, y, w, h, 5); fillColor(on ? withAlpha(acc, 0.45f) : (hot ? withAlpha(acc, 0.18f) : C_PANEL_HI)); fill();
        strokeColor(on || hot ? acc : C_LINE); strokeWidth(1); stroke();
        fontSize(9); textLetterSpacing(2.f); textAlign(ALIGN_CENTER | ALIGN_MIDDLE); fillColor(on || hot ? C_TEXT : C_DIM);
        text(x + w / 2, y + h / 2 + 0.5f, label, nullptr);
    }
    void drawDropButton(float x, float y, float w, float h, const char* label, bool hot) {
        beginPath(); roundedRect(x, y, w, h, 4); fillColor(hot ? C_PANEL_HI : C_PANEL_LO); fill(); strokeColor(hot ? C_DIM : C_LINE); strokeWidth(1); stroke();
        fontSize(8); textLetterSpacing(1.2f); textAlign(ALIGN_LEFT | ALIGN_MIDDLE); fillColor(hot ? C_TEXT : C_DIM); text(x + 10, y + h / 2 + 0.5f, label, nullptr);
        beginPath(); const float ax = x + w - 12, ay = y + h / 2; moveTo(ax - 3.5f, ay - 2); lineTo(ax + 3.5f, ay - 2); lineTo(ax, ay + 2.5f); closePath(); fillColor(C_DIM); fill();
    }
    void drawHeaderButton(float x, float w, const char* label, bool on, bool hot, bool disabled = false) {
        beginPath(); roundedRect(x, PS_Y, w, PS_H, 5); fillColor(on ? withAlpha(C_ACC, 0.22f) : C_PANEL); fill();
        strokeColor(on ? C_ACC : (hot && !disabled ? C_DIM : C_LINE)); strokeWidth(1); stroke();
        fontSize(9); textLetterSpacing(1.6f); textAlign(ALIGN_CENTER | ALIGN_MIDDLE); fillColor(disabled ? C_FAINT : (on ? C_TEXT : C_DIM));
        text(x + w / 2, PS_Y + PS_H / 2 + 0.5f, label, nullptr);
    }
    void drawHeader() {
        const Color bands[4] = { C_ACC, Color(0xc4, 0x7e, 0x46), Color(0x9c, 0x6a, 0x48), Color(0x6e, 0x58, 0x4a) };
        for (int i = 0; i < 4; ++i) {
            beginPath(); const float y0 = 19 + i * 7.5f;
            moveTo(17, y0 + 1.5f * std::sin(i * 1.3f)); bezierTo(24, y0 - 2, 30, y0 + 3, 38, y0 + 0.5f * i);
            lineTo(38, y0 + 6 + 0.5f * i); bezierTo(30, y0 + 9, 24, y0 + 4, 17, y0 + 6 + 1.5f * std::sin(i * 1.3f)); closePath();
            fillColor(bands[i]); fill();
        }
        fontSize(22); textLetterSpacing(9); fillColor(C_TEXT); textAlign(ALIGN_LEFT | ALIGN_MIDDLE); text(50, 34, "STRATA", nullptr);
        fontSize(9.5f); textLetterSpacing(2.5f); fillColor(C_DIM); text(216, 29, "STEREO TAPE DELAY", nullptr);
        fontSize(7); textLetterSpacing(1.1f); fillColor(C_FAINT); text(216, 41, "EPSILON AUDIO 3.1", nullptr);
        drawPresetBar();
        for (int k = 0; k < 2; ++k) {
            const float x = A_X + k * AB_W; const bool on = abSlot == k, hot = hoverHeader == 6 + k;
            beginPath(); roundedRect(x, PS_Y, AB_W - 2, PS_H, 5); fillColor(on ? withAlpha(C_ACC, 0.25f) : C_PANEL); fill();
            strokeColor(on ? C_ACC : (hot ? C_DIM : C_LINE)); strokeWidth(1); stroke();
            fontSize(10); textLetterSpacing(0); textAlign(ALIGN_CENTER | ALIGN_MIDDLE); fillColor(on ? C_TEXT : C_DIM);
            text(x + (AB_W - 2) / 2, PS_Y + PS_H / 2 + 0.5f, k == 0 ? "A" : "B", nullptr);
        }
        drawHeaderButton(COPY_X, COPY_W, "COPY", false, hoverHeader == 8);
        drawHeaderButton(UNDO_X, UR_W, "UNDO", false, hoverHeader == 3, undoStack.empty());
        drawHeaderButton(REDO_X, UR_W, "REDO", false, hoverHeader == 4, redoStack.empty());
        drawHeaderButton(LEARN_X, LEARN_W, learnMode ? "LEARNING" : "LEARN", learnMode, hoverHeader == 1);
        drawHeaderButton(MIDI_X, MIDI_W, "MIDI", val[kMidiOn] > 0.5f, hoverHeader == 2);
        drawHeaderButton(HELP_X, HELP_W, "HELP", helpOpen, hoverHeader == 5);
    }

    // ---- left / right ----------------------------------------------------------
    void drawSides() {
        panel(LX, R1, SIDEW, R1H, "LEFT"); panel(RX, R1, SIDEW, R1H, "RIGHT");
        const bool ms = val[kChanMode] > 0.5f;
        for (int s = 0; s < 2; ++s) {
            const float x = s == 0 ? LX : RX;
            if (ms) { beginPath(); rect(x + 12, R1 + 6, 70, 18); fillColor(C_PANEL); fill();
                      fontSize(9.5f); textLetterSpacing(2.2f); fillColor(C_DIM); textAlign(ALIGN_LEFT | ALIGN_TOP); text(x + 14, R1 + 10, s == 0 ? "MID" : "SIDE", nullptr); }
            const float mx = s == 0 ? (ms ? 50 : 52) : (ms ? 54 : 58);
            beginPath(); roundedRect(x + mx, R1 + 11, 16, 7, 3.5f); fillColor(s == 0 ? C_LEFT : C_RIGHT); fill();
            char b[32]; fmtMs(val[s == 0 ? kOutTimeL : kOutTimeR], b, sizeof b);
            fontSize(10); textLetterSpacing(0.8f); textAlign(ALIGN_LEFT | ALIGN_TOP); fillColor(s == 0 ? C_LEFT : C_RIGHT);
            text(x + mx + 28, R1 + 9, b, nullptr);
        }
    }

    // ---- echo map / tap editor ------------------------------------------------------
    struct Echo { float t, a; int ch; bool tap; };
    void echoEvents(std::vector<Echo>& out, float window) const {
        const float T[2] = { std::max(val[kOutTimeL], 0.5f), std::max(val[kOutTimeR], 0.5f) };
        const float g = std::min(1.2f, std::abs(loopH(val, 1000.f)));
        const float inv[2] = { val[kInvL] > 0.5f ? -1.f : 1.f, val[kInvR] > 0.5f ? -1.f : 1.f };
        const float fb[2] = { val[kFbL], val[kFbR] }, xf[2] = { val[kXfLR], val[kXfRL] };
        const float main = val[kMainOn] > 0.5f ? dbToGain(val[kMainLevel]) : 0.f;
        const bool frz = val[kFreeze] > 0.5f;
        struct Ev { float start, a; int ch; };          // a write into line ch at time start
        std::vector<Ev> q; const int mode = (int)(val[kInput] + .5f);
        if (mode != 3) q.push_back({ 0, 1, 0 });
        if (mode != 2) q.push_back({ 0, 1, 1 });
        for (size_t i = 0; i < q.size() && out.size() < 600; ++i) {
            const Ev e = q[i];
            for (int t = 0; t < kNumTaps; ++t) {          // the extra taps read this write as it passes (output only)
                if (val[tpar(t, kTOn)] < 0.5f) continue;
                const float pn = val[tpar(t, kTPan)], side = e.ch == 0 ? (pn > 0 ? 1.f - pn : 1.f) : (pn < 0 ? 1.f + pn : 1.f);
                const float ta = e.a * dbToGain(val[tpar(t, kTLevel)]) * side, tt = e.start + val[tpar(t, kTTime)] * T[e.ch];
                if (tt <= window && std::fabs(ta) >= 0.015f) out.push_back({ tt, ta, e.ch, true });
            }
            const float t = e.start + T[e.ch], a = e.a * (frz ? 1.f : g);
            if (t > window || std::fabs(a) < 0.015f) continue;
            if (main > 0) out.push_back({ t, a * main, e.ch, false });
            if (q.size() < 800) {
                if (frz) { q.push_back({ t, a, e.ch }); continue; }
                if (fb[e.ch] > 0) q.push_back({ t, a * fb[e.ch] * inv[e.ch], e.ch });
                if (xf[e.ch] > 0) q.push_back({ t, a * xf[e.ch] * inv[1 - e.ch], 1 - e.ch });
            }
        }
    }
    void drawEchoPanel() {
        panel(EX, R1, ECHOW, R1H, tapView ? "TAPS" : "ECHOES");
        // view switch
        static const char* const vl[2] = { "MAP", "TAPS" };
        for (int i = 0; i < 2; ++i) {
            const float sw = VIEW_W / 2, x = VIEW_X + sw * i; const bool on = (int)tapView == i;
            beginPath(); roundedRect(x + 1, VIEW_Y - 9, sw - 2, 18, 4); fillColor(on ? C_ACC : (hoverView == i ? C_PANEL_HI : C_PANEL_LO)); fill();
            fontSize(8); textLetterSpacing(1.3f); textAlign(ALIGN_CENTER | ALIGN_MIDDLE); fillColor(on ? C_BG : C_DIM); text(x + sw / 2, VIEW_Y + 0.5f, vl[i], nullptr);
        }
        if (tapView) drawDropButton(PAT_X, PAT_Y, PAT_W, PAT_H, "PATTERNS", hoverPat || (popup.open && popup.kind == 6));
        if (tapView) drawTapEditor(); else drawEchoes();
        caption(EX + 80, R1 + 200, "MODE"); caption(EX + 330, R1 + 200, "INPUT");
        const bool frozen = val[kFreeze] > 0.5f || liveFrozen;
        drawButton(FRZ_X, FRZ_Y, FRZ_W, FRZ_H, "FREEZE", frozen, hoverFrz, C_FREEZE);
        drawButton(TAP_X, TAP_Y, TAP_W, TAP_H, tapFlash > 0 ? "TAP" : "TAP TEMPO", tapFlash > 0, hoverTap);
        if (learnMode && (paramCC[kFreeze] >= 0 || learnWaiting == (int)kFreeze)) learnBadge(FRZ_X + FRZ_W / 2, FRZ_Y - 8, kFreeze);
    }
    void drawEchoes() {
        const float x = DISP_X, y = DISP_Y, w = DISP_W, h = DISP_H, mid = y + h / 2;
        displayBox(x, y, w, h);
        const float maxT = std::max(val[kOutTimeL], val[kOutTimeR]);
        static const float nice[] = { 10, 20, 50, 100, 200, 500, 1000, 2000, 4000, 8000, 16000 };
        float window = 16000; for (float n : nice) if (n >= maxT * 4.5f) { window = n; break; }
        const float step = window / 10;
        fontSize(7.5f); textLetterSpacing(0.6f); textAlign(ALIGN_CENTER | ALIGN_BOTTOM);
        for (int i = 0; i <= 10; ++i) {
            const float gx = x + 8 + (w - 16) * i / 10.f;
            beginPath(); moveTo(gx, y + 6); lineTo(gx, y + h - 14); strokeColor(withAlpha(C_LINE, i % 5 == 0 ? 0.9f : 0.45f)); strokeWidth(1); stroke();
            if (i % 2 == 0) { char b[16]; const float t = step * i; if (window >= 2000) snprintf(b, sizeof b, "%.1fs", t / 1000); else snprintf(b, sizeof b, "%.0f", t); fillColor(C_FAINT); text(gx, y + h - 3, b, nullptr); }
        }
        beginPath(); moveTo(x + 8, mid); lineTo(x + w - 8, mid); strokeColor(C_LINE); stroke();
        const bool ms = val[kChanMode] > 0.5f;
        fontSize(7.5f); textLetterSpacing(1.5f); textAlign(ALIGN_LEFT | ALIGN_TOP); fillColor(withAlpha(C_LEFT, 0.7f)); text(x + 10, y + 6, ms ? "M" : "L", nullptr);
        textAlign(ALIGN_LEFT | ALIGN_BOTTOM); fillColor(withAlpha(C_RIGHT, 0.7f)); text(x + 10, y + h - 16, ms ? "S" : "R", nullptr);
        beginPath(); moveTo(x + 8, mid - (h / 2 - 16)); lineTo(x + 8, mid + (h / 2 - 22)); strokeColor(withAlpha(C_TEXT, 0.5f)); strokeWidth(2); stroke();
        std::vector<Echo> ev; echoEvents(ev, window);
        const float amp = h / 2 - 18;
        float peak = 1.f; for (const Echo& e : ev) peak = std::max(peak, std::fabs(e.a));
        for (int pass = 0; pass < 2; ++pass) for (const Echo& e : ev) {
            if (e.tap != (pass == 0)) continue;
            const float tx = x + 8 + (w - 16) * e.t / window, len = std::fabs(e.a) / peak * amp;
            const Color c = e.ch == 0 ? C_LEFT : C_RIGHT;
            const float y1 = e.ch == 0 ? mid - len : mid + len;
            beginPath(); moveTo(tx, mid); lineTo(tx, y1); strokeColor(withAlpha(c, e.tap ? 0.45f : 0.9f)); strokeWidth(e.tap ? 1.2f : 2.2f); lineCap(ROUND); stroke();
            if (!e.tap) { beginPath(); circle(tx, y1, 2.6f); if (e.a < 0) { strokeColor(c); strokeWidth(1.2f); stroke(); } else { fillColor(c); fill(); } }
        }
        std::string l = sideText(0), r = sideText(1);
        fontSize(9); textLetterSpacing(0.5f); textAlign(ALIGN_RIGHT | ALIGN_TOP);
        fillColor(C_LEFT); text(x + w - 10, y + 6, l.c_str(), nullptr);
        textAlign(ALIGN_RIGHT | ALIGN_BOTTOM); fillColor(C_RIGHT); text(x + w - 10, y + h - 16, r.c_str(), nullptr);
        if (val[kFreeze] > 0.5f || liveFrozen) { fontSize(8); textLetterSpacing(2.f); textAlign(ALIGN_CENTER | ALIGN_TOP); fillColor(C_FREEZE); text(x + w / 2, y + 6, "FROZEN", nullptr); }
    }
    std::string sideText(int c) const {
        const int s = linked() ? 0 : c;
        char b[64], t[24]; fmtMs(val[c == 0 ? kOutTimeL : kOutTimeR], t, sizeof t);
        const int sy = (int)(val[kSyncL + s] + .5f);
        if (sy == 0) snprintf(b, sizeof b, "%s", t);
        else { char d[24]; fmt(kDivL + s, d, sizeof d); snprintf(b, sizeof b, "%s  ·  %s", d, t); }
        return std::string(c == 1 && linked() ? "LINKED  " : "") + b;
    }
    // tap editor geometry: time 0..1 across, level -60..+6 dB up
    static float txOf(float t) { return DISP_X + 16 + (DISP_W - 40) * t; }
    static float tOfX(float x) { return clampf((x - DISP_X - 16) / (DISP_W - 40), 0.01f, 1.f); }
    static float tyOf(float db) { const float base = DISP_Y + DISP_H - 16, top = DISP_Y + 16; return base - (clampf(db, -60.f, 6.f) + 60.f) / 66.f * (base - top); }
    static float dbOfY(float y) { const float base = DISP_Y + DISP_H - 16, top = DISP_Y + 16; return clampf((base - y) / (base - top) * 66.f - 60.f, -60.f, 6.f); }
    Color panColor(float pn, float a) const {
        const Color l = C_LEFT, r = C_RIGHT, c = C_TEXT;
        const float u = std::fabs(pn); const Color& e = pn < 0 ? l : r;
        return Color(c.red + (e.red - c.red) * u, c.green + (e.green - c.green) * u, c.blue + (e.blue - c.blue) * u, a);
    }
    void drawTapEditor() {
        const float x = DISP_X, y = DISP_Y, w = DISP_W, h = DISP_H, base = y + h - 16;
        displayBox(x, y, w, h);
        fontSize(7.5f); textLetterSpacing(0.6f); textAlign(ALIGN_CENTER | ALIGN_BOTTOM);
        for (int i = 0; i <= 16; ++i) {
            const float gx = txOf(i / 16.f);
            beginPath(); moveTo(gx, y + 8); lineTo(gx, base); strokeColor(withAlpha(C_LINE, i % 4 == 0 ? 0.9f : 0.35f)); strokeWidth(1); stroke();
            static const char* const lab[5] = { "0", "1/4", "1/2", "3/4", "1" };
            if (i % 4 == 0) { fillColor(C_FAINT); text(gx, y + h - 3, lab[i / 4], nullptr); }
        }
        for (int db = -48; db <= 0; db += 12) { const float gy = tyOf((float)db); beginPath(); moveTo(x + 8, gy); lineTo(x + w - 8, gy); strokeColor(withAlpha(C_LINE, db == 0 ? 0.8f : 0.3f)); stroke(); }
        fontSize(7); textAlign(ALIGN_LEFT | ALIGN_MIDDLE); fillColor(C_FAINT); text(x + 4, tyOf(0) - 5, "0 dB", nullptr);
        // extra taps
        for (int t = 0; t < kNumTaps; ++t) {
            if (val[tpar(t, kTOn)] < 0.5f) continue;
            const float tx = txOf(val[tpar(t, kTTime)]), ty = tyOf(val[tpar(t, kTLevel)]), pn = val[tpar(t, kTPan)];
            const bool hot = t == hoverTapIdx || t == dragTap, sel = t == selTap;
            beginPath(); moveTo(tx, base); lineTo(tx, ty); strokeColor(panColor(pn, hot ? 1.f : 0.8f)); strokeWidth(hot ? 3.f : 2.2f); lineCap(ROUND); stroke();
            beginPath(); circle(tx, ty, hot ? 6.f : 5.f); fillColor(panColor(pn, 1.f)); fill();
            if (sel) { beginPath(); circle(tx, ty, 9); strokeColor(withAlpha(C_TEXT, 0.6f)); strokeWidth(1.2f); stroke(); }
            char b[4]; snprintf(b, sizeof b, "%d", t + 1); fontSize(7.5f); textLetterSpacing(0); textAlign(ALIGN_CENTER | ALIGN_BOTTOM); fillColor(C_DIM); text(tx, ty - 8, b, nullptr);
        }
        // main tap
        { const bool on = val[kMainOn] > 0.5f; const float tx = txOf(1.f), ty = tyOf(val[kMainLevel]); const bool hot = hoverTapIdx == 100 || dragTap == 100;
          beginPath(); moveTo(tx, base); lineTo(tx, ty); strokeColor(on ? C_ACC : C_FAINT); strokeWidth(hot ? 4.5f : 3.5f); lineCap(ROUND); stroke();
          beginPath(); rect(tx - 6, ty - 6, 12, 12); if (on) { fillColor(C_ACC); fill(); } else { strokeColor(C_FAINT); strokeWidth(1.4f); stroke(); }
          fontSize(7.5f); textLetterSpacing(1); textAlign(ALIGN_RIGHT | ALIGN_BOTTOM); fillColor(on ? C_ACC : C_FAINT); text(tx - 9, ty - 4, "MAIN", nullptr); }
        // readout
        std::string info;
        const int t = dragTap >= 0 ? dragTap : (hoverTapIdx >= 0 ? hoverTapIdx : selTap);
        char b[96];
        if (t == 100) { char lv[24], pn[24]; fmt(kMainLevel, lv, sizeof lv); fmt(kMainPan, pn, sizeof pn); snprintf(b, sizeof b, "MAIN TAP   %s   %s%s", lv, pn, val[kMainOn] > 0.5f ? "" : "   (OFF)"); info = b; }
        else if (t >= 0 && t < kNumTaps && val[tpar(t, kTOn)] > 0.5f) {
            char lv[24], pn[24], ms[24]; fmt(tpar(t, kTLevel), lv, sizeof lv); fmt(tpar(t, kTPan), pn, sizeof pn); fmtMs(val[tpar(t, kTTime)] * val[kOutTimeL], ms, sizeof ms);
            snprintf(b, sizeof b, "TAP %d   %.1f%%  (%s)   %s   %s", t + 1, val[tpar(t, kTTime)] * 100, ms, lv, pn); info = b;
        } else { int n = 0; for (int k = 0; k < kNumTaps; ++k) n += val[tpar(k, kTOn)] > 0.5f; snprintf(b, sizeof b, "%d / %d TAPS   ·   DOUBLE-CLICK TO ADD", n, kNumTaps); info = b; }
        fontSize(8.5f); textLetterSpacing(0.8f); textAlign(ALIGN_LEFT | ALIGN_TOP); fillColor(C_DIM); text(x + 10, y + 6, info.c_str(), nullptr);
        char tl[24]; fmtMs(val[kOutTimeL], tl, sizeof tl); fontSize(8.5f); textAlign(ALIGN_RIGHT | ALIGN_TOP); fillColor(C_LEFT); text(x + w - 10, y + 6, tl, nullptr);
    }
    int tapHit(float px, float py) const {
        if (!tapView || !inRect(px, py, DISP_X, DISP_Y, DISP_W, DISP_H)) return -1;
        const float base = DISP_Y + DISP_H - 16;
        { const float tx = txOf(1.f), ty = tyOf(val[kMainLevel]); if (std::fabs(px - tx) <= 7 && py >= ty - 8 && py <= base + 3) return 100; }
        int best = -1; float bd = 1e9f;
        for (int t = 0; t < kNumTaps; ++t) {
            if (val[tpar(t, kTOn)] < 0.5f) continue;
            const float tx = txOf(val[tpar(t, kTTime)]), ty = tyOf(val[tpar(t, kTLevel)]);
            if (std::fabs(px - tx) > 7 || py < ty - 8 || py > base + 3) continue;
            const float d = std::fabs(px - tx) + 0.2f * std::fabs(py - ty);
            if (d < bd) { bd = d; best = t; }
        }
        return best;
    }
    void addTapAt(float px, float py) {
        int t = -1; for (int k = 0; k < kNumTaps; ++k) if (val[tpar(k, kTOn)] < 0.5f) { t = k; break; }
        if (t < 0) { flash("All 15 taps are on - right-click one to remove it"); return; }
        beginGesture(); commit(tpar(t, kTTime), tOfX(px)); commit(tpar(t, kTLevel), std::round(dbOfY(py) * 2.f) / 2.f); commit(tpar(t, kTOn), 1); endGesture();
        selTap = t; char b[48]; snprintf(b, sizeof b, "Tap %d on", t + 1); flash(b);
    }

    // ---- filters --------------------------------------------------------------------
    void drawFilterPanel() {
        panel(FX0, R2, FW0, R2H);
        for (int f = 0; f < kNumFilters; ++f) {
            const float x = FT_X + f * (FT_W + 4); const bool sel = f == selF, on = fOn(f), hot = hoverFTab == f;
            const Color col = filterColor(f);
            beginPath(); roundedRect(x, FT_Y, FT_W, FT_H, 5); fillColor(sel ? withAlpha(col, 0.22f) : (hot ? C_PANEL_HI : C_PANEL_LO)); fill();
            strokeColor(sel ? col : C_LINE); strokeWidth(1); stroke();
            beginPath(); circle(x + 13, FT_Y + FT_H / 2, 5); fillColor(on ? col : C_PANEL_HI); fill(); strokeColor(withAlpha(col, 0.8f)); stroke();
            char t[8]; snprintf(t, sizeof t, "F%d", f + 1);
            fontSize(10.5f); textLetterSpacing(0.5f); textAlign(ALIGN_CENTER | ALIGN_MIDDLE); fillColor(sel ? C_TEXT : C_DIM); text(x + 35, FT_Y + FT_H / 2 + 0.5f, t, nullptr);
            if (learnMode && (paramCC[fpar(f, kFOn)] >= 0 || learnWaiting == (int)fpar(f, kFOn))) learnBadge(x + FT_W / 2, FT_Y - 6, fpar(f, kFOn));
        }
        const Color col = filterColor(selF);
        beginPath(); roundedRect(STYLE_X, STYLE_Y, STYLE_W, 26, 5); fillColor(hoverStyle ? C_PANEL_HI : C_PANEL_LO); fill(); strokeColor(withAlpha(col, 0.8f)); strokeWidth(1); stroke();
        const int st = (int)std::round(val[fpar(selF, kFStyle)]);
        fontSize(7.5f); textLetterSpacing(1.4f); textAlign(ALIGN_LEFT | ALIGN_MIDDLE); fillColor(C_FAINT); text(STYLE_X + 10, STYLE_Y + 13.5f, "STYLE", nullptr);
        fontSize(11); textLetterSpacing(0.6f); fillColor(C_TEXT); textAlign(ALIGN_RIGHT | ALIGN_MIDDLE); text(STYLE_X + STYLE_W - 24, STYLE_Y + 13.5f, styleName(st), nullptr);
        beginPath(); moveTo(STYLE_X + STYLE_W - 15, STYLE_Y + 11); lineTo(STYLE_X + STYLE_W - 8, STYLE_Y + 11); lineTo(STYLE_X + STYLE_W - 11.5f, STYLE_Y + 16); closePath(); fillColor(C_DIM); fill();
        { char b[32]; snprintf(b, sizeof b, "%d OF 6 ON", filtersOn()); fontSize(8); textLetterSpacing(1.4f); textAlign(ALIGN_RIGHT | ALIGN_MIDDLE); fillColor(C_FAINT); text(FX0 + FW0 - 14, STYLE_Y + 13.5f, b, nullptr); }
        caption(FT_X + 161, R2 + 48, "SHAPE"); caption(FT_X + 434, R2 + 48, "SLOPE");
        caption(400, R2 + 184, "ROUTING", ALIGN_RIGHT);
        // response
        displayBox(FD_X, FD_Y, FD_W, FD_H);
        const int N = 140;
        auto px = [&](int i) { return FD_X + 4 + (FD_W - 8) * i / (float)N; };
        auto hzAt = [&](int i) { return 20.f * std::pow(1000.f, (float)i / N); };
        auto py = [&](float db) { return FD_Y + FD_H - 4 - (FD_H - 8) * (clampf(db, -30.f, 24.f) + 30.f) / 54.f; };
        for (const float g : { 100.f, 1000.f, 10000.f }) { const float gx = FD_X + 4 + (FD_W - 8) * std::log(g / 20.f) / std::log(1000.f); beginPath(); moveTo(gx, FD_Y + 4); lineTo(gx, FD_Y + FD_H - 4); strokeColor(Color(255, 255, 255, 8 / 255.f)); stroke(); }
        beginPath(); moveTo(FD_X + 4, py(0)); lineTo(FD_X + FD_W - 4, py(0)); strokeColor(Color(255, 255, 255, 14 / 255.f)); stroke();
        scissor(FD_X + 1, FD_Y + 1, FD_W - 2, FD_H - 2);
        for (int f = kNumFilters - 1; f >= 0; --f) {
            if (!fOn(f) && f != selF) continue;
            beginPath(); for (int i = 0; i <= N; ++i) { const float yy = py(20 * std::log10(std::max(std::abs(filterH(val, f, hzAt(i))), 1e-4f))); if (!i) moveTo(px(i), yy); else lineTo(px(i), yy); }
            strokeColor(withAlpha(filterColor(f), fOn(f) ? (f == selF ? 0.95f : 0.45f) : 0.2f)); strokeWidth(f == selF ? 1.8f : 1.1f); stroke();
        }
        if (filtersOn() >= 2) {
            const bool pc = (int)(val[kRouting] + .5f) == kPerChannel;
            for (int side = pc ? 0 : -1; side <= (pc ? 1 : -1); ++side) {
                beginPath(); for (int i = 0; i <= N; ++i) { const float yy = py(20 * std::log10(std::max(std::abs(loopH(val, hzAt(i), side)), 1e-4f))); if (!i) moveTo(px(i), yy); else lineTo(px(i), yy); }
                strokeColor(side == 1 ? withAlpha(C_RIGHT, 0.8f) : Color(0xee, 0xe6, 0xda, 0.75f)); strokeWidth(1.2f); stroke();
            }
        }
        resetScissor();
        if (!fOn(selF)) { fontSize(8); textLetterSpacing(1.2f); textAlign(ALIGN_CENTER | ALIGN_MIDDLE); fillColor(C_FAINT); text(FD_X + FD_W / 2, FD_Y + 14, "OFF - CLICK THE DOT ON ITS TAB", nullptr); }
    }

    // ---- effects / output ------------------------------------------------------------
    void drawEffectsPanel() {
        panel(EFX, R2, EFW, R2H);
        fontSize(9.5f); textLetterSpacing(2.2f); fillColor(C_DIM); textAlign(ALIGN_LEFT | ALIGN_MIDDLE); text(EFX + 14, R2 + 40, "", nullptr);
        caption(effCol(0), R2 + 40, "SATURATION");
        for (int i = 1; i < 5; ++i) { beginPath(); moveTo(effCol(i) - 51, R2 + 24); lineTo(effCol(i) - 51, R2 + R2H - 14); strokeColor(C_LINE); strokeWidth(1); stroke(); }
    }
    void drawOutputPanel() {
        panel(OX, OY, OW, OH, "OUTPUT");
        caption(OX + 136, OY + 30, "CHANNELS");
        drawButton(CLR2_X, CLR2_Y, CLR2_W, CLR2_H, "CLEAR ECHOES", false, hoverClr2);
    }

    // ---- modulation sources --------------------------------------------------------
    static const char* tabName(int t) { static const char* const n[kNumTabs] = { "LFO 1", "LFO 2", "LFO 3", "LFO 4", "EG 1", "EG 2", "EF 1", "EF 2", "XY 1", "XY 2", "MIDI", "CLASSIC" }; return n[t]; }
    static int tabSource(int t) { static const int s[kNumTabs] = { em::kSrcXlfo1, em::kSrcXlfo2, em::kSrcXlfo3, em::kSrcXlfo4, em::kSrcEg1, em::kSrcEg2, em::kSrcEf1, em::kSrcEf2, em::kSrcXy1X, em::kSrcXy2X, em::kSrcVelocity, kClassicChip }; return s[t]; }
    static float tabX(int t) { return MX + 10 + t * (TAB_W + 1.6f); }
    struct Chip { int src; float x, y; };
    std::vector<Chip> chips() const {
        std::vector<Chip> c;
        if (tab <= 7) c.push_back({ tabSource(tab), MX + MW - CHIP_W - 14, MY + 52 });
        else if (tab <= 9) { const int x = tab - 8; c.push_back({ em::kSrcXy1X + 2 * x, MX + MW - CHIP_W - 14, MY + 52 }); c.push_back({ em::kSrcXy1Y + 2 * x, MX + MW - CHIP_W - 14, MY + 78 }); }
        else if (tab == 10) for (int i = 0; i < 5; ++i) c.push_back({ em::kSrcVelocity + i, MX + 24, MY + 52 + i * 40.f });
        else for (int i = 0; i < 3; ++i) c.push_back({ kClassicChip + i, MX + 14 + i * CL_COLW + 112, MY + 40 });
        return c;
    }
    bool sourceUsed(int s) const {
        if (s >= kClassicChip) { const int cs = kSrcLfo1 + (s - kClassicChip);
            for (int k = 0; k < kNumSlots; ++k) if ((int)std::round(val[slotParam(k, kSlotSrc)]) == cs && (int)std::round(val[slotParam(k, kSlotDst)]) > 0) return true;
            return false; }
        for (int k = 0; k < kNumSlots2; ++k) if ((int)std::round(val[sp(k, kSlotSrc)]) == s && (int)std::round(val[sp(k, kSlotDst)]) > 0) return true;
        return false;
    }
    bool anyMidiUsed() const { for (int s = em::kSrcVelocity; s < em::kNumSources; ++s) if (sourceUsed(s)) return true; return false; }
    bool anyClassicUsed() const { for (int s = 0; s < kNumSlots; ++s) if (classicUsed(s)) return true; return false; }
    void drawSources() {
        panel(MX, MY, MW, MH);
        for (int t = 0; t < kNumTabs; ++t) {
            const float x = tabX(t); const bool on = t == tab, hot = t == hoverTab;
            const int s = tabSource(t);
            beginPath(); roundedRect(x, TAB_Y - TAB_H / 2, TAB_W, TAB_H, 4); fillColor(on ? withAlpha(srcColor(s), 0.25f) : (hot ? C_PANEL_HI : C_PANEL_LO)); fill();
            if (on) { strokeColor(srcColor(s)); strokeWidth(1); stroke(); }
            fontSize(7.5f); textLetterSpacing(t == kTabClassic ? 0.4f : 0.8f); textAlign(ALIGN_CENTER | ALIGN_MIDDLE); fillColor(on ? C_TEXT : C_DIM); text(x + TAB_W / 2, TAB_Y + 0.5f, tabName(t), nullptr);
            const bool used = t == kTabClassic ? anyClassicUsed() : (sourceUsed(s) || (t >= 8 && t <= 9 && sourceUsed(s + 1)) || (t == 10 && anyMidiUsed()));
            if (used) { beginPath(); circle(x + TAB_W - 5, TAB_Y - TAB_H / 2 + 4, 2); fillColor(srcColor(s)); fill(); }
        }
        if (tab <= 3) drawXlfo(tab);
        else if (tab <= 5) drawEg(tab - 4);
        else if (tab <= 7) drawEf(tab - 6);
        else if (tab <= 9) drawXy(tab - 8);
        else if (tab == 10) drawMidi();
        else drawClassic();
        for (const Chip& c : chips()) drawChip(c.src, c.x, c.y, hoverChip == c.src, false);
        if (tab <= 7) drawDropButton(COMP_X, COMP_Y, COMP_W, COMP_H, tab <= 3 ? "LFO PRESETS" : (tab <= 5 ? "EG PRESETS" : "EF PRESETS"), hoverComp || (popup.open && popup.kind == 3));
    }
    void drawChip(int src, float x, float y, bool hot, bool dragging) {
        const Color col = srcColor(src);
        beginPath(); roundedRect(x, y, CHIP_W, CHIP_H, CHIP_H / 2); fillColor(withAlpha(col, dragging ? 0.9f : (hot ? 0.35f : 0.16f))); fill(); strokeColor(col); strokeWidth(1); stroke();
        const float v = src >= kClassicChip ? liveClassic[kSrcLfo1 + src - kClassicChip] : liveSrc[src];
        const bool bip = src >= kClassicChip ? src < kClassicChip + 2 : em::sourceBipolar(src);
        beginPath(); circle(x + 9, y + CHIP_H / 2, 3); fillColor(dragging ? C_BG : col); fill();
        if (!dragging) { const float bx = x + 17, bw = CHIP_W - 24, by = y + CHIP_H - 4;
            beginPath(); rect(bx, by, bw, 1.5f); fillColor(withAlpha(col, 0.25f)); fill();
            const float a = bip ? bx + bw / 2 : bx, b2 = bip ? bx + bw / 2 + clampf(v, -1.f, 1.f) * bw / 2 : bx + clampf(v, 0.f, 1.f) * bw;
            beginPath(); rect(std::min(a, b2), by, std::fabs(b2 - a) + 1, 1.5f); fillColor(col); fill(); }
        fontSize(8); textLetterSpacing(1.1f); textAlign(ALIGN_CENTER | ALIGN_MIDDLE); fillColor(dragging ? C_BG : C_TEXT); text(x + CHIP_W / 2 + 5, y + CHIP_H / 2 - 0.5f, chipShort(src), nullptr);
    }
    void drawXlfo(int l) {
        caption(MX + 82, MY + 44, "MODE"); caption(MX + 296, MY + 44, "TEMPO SYNC");
        const bool steps = val[lp_(l, em::kLMode)] > 0.5f;
        displayBox(STEP_X, STEP_Y, STEP_W, STEP_H);
        const float mid = STEP_Y + STEP_H / 2; const Color col = srcColor(em::kSrcXlfo1);
        beginPath(); moveTo(STEP_X, mid); lineTo(STEP_X + STEP_W, mid); strokeColor(Color(255, 255, 255, 10 / 255.f)); stroke();
        if (steps) {
            const int n = (int)clampf(std::round(val[lp_(l, em::kLSteps)]), 1, kNumSteps); const float sw = STEP_W / kNumSteps;
            for (int s = 0; s < kNumSteps; ++s) {
                const float v = val[lp_(l, em::kLStep1 + s)], x = STEP_X + s * sw, y = mid - v * (STEP_H / 2 - 6);
                beginPath(); rect(x + 2, std::min(y, mid), sw - 4, std::max(1.5f, std::fabs(y - mid))); fillColor(withAlpha(col, s < n ? 0.75f : 0.15f)); fill();
            }
            const float ph = (float)livePhase[l]; beginPath(); const float px = STEP_X + ph * n * sw; moveTo(px, STEP_Y + 2); lineTo(px, STEP_Y + STEP_H - 2); strokeColor(withAlpha(C_TEXT, 0.6f)); stroke();
        } else {
            beginPath();
            const int w = (int)std::round(val[lp_(l, em::kLWave)]); const float bal = val[lp_(l, em::kLBalance)];
            em::Xlfo tmp; tmp.rndA = -0.6f; tmp.rndB = 0.5f; tmp.held = 0.4f;
            for (int i = 0; i <= 120; ++i) { tmp.phase = i / 120.0; const float v = (w == 5 || w == 6) ? std::sin(i * 0.31f) * 0.6f * ((i / 20) % 2 ? 1 : -0.7f) : tmp.wave(w, bal);
                const float x = STEP_X + 4 + (STEP_W - 8) * i / 120.f, y = mid - v * (STEP_H / 2 - 8); if (!i) moveTo(x, y); else lineTo(x, y); }
            strokeColor(col); strokeWidth(1.6f); stroke();
            beginPath(); circle(STEP_X + 4 + (STEP_W - 8) * (float)livePhase[l], mid - liveSrc[em::kSrcXlfo1 + l] * (STEP_H / 2 - 8), 3.5f); fillColor(C_TEXT); fill();
        }
    }
    void drawAdsr(float x, float y, float w, float h, float a, float d, float s, float r, const Color& col, float live) {
        displayBox(x, y, w, h);
        const float sus = 0.4f, tot = std::max(a + d + sus + r, 0.3f), sx = (w - 16) / tot, x0 = x + 5, yb = y + h - 4, hh = h - 8;
        auto Y = [&](float lvl) { return yb - hh * lvl; };
        beginPath(); moveTo(x0, Y(0)); lineTo(x0 + a * sx, Y(1));
        for (int i = 1; i <= 16; ++i) { const float t = i / 16.f; lineTo(x0 + (a + d * t) * sx, Y(s + (1 - s) * std::exp(-4 * t))); }
        lineTo(x0 + (a + d + sus) * sx, Y(s));
        for (int i = 1; i <= 16; ++i) { const float t = i / 16.f; lineTo(x0 + (a + d + sus + r * t) * sx, Y(s * std::exp(-4 * t))); }
        strokeColor(col); strokeWidth(1.4f); stroke();
        const float e = clampf(live, 0.f, 1.f);
        beginPath(); roundedRect(x + w - 8, y + 4, 4, h - 8, 2); fillColor(C_PANEL_HI); fill();
        if (e > 0.002f) { beginPath(); roundedRect(x + w - 8, yb - hh * e, 4, hh * e, 2); fillColor(col); fill(); }
    }
    void drawEg(int e) {
        caption(MX + 116, MY + 44, "TRIGGER");
        drawAdsr(MX + 16, MY + 214, 420, 68, val[ep(e, em::kEAttack)], val[ep(e, em::kEDecay)], val[ep(e, em::kESustain)], val[ep(e, em::kERelease)], srcColor(em::kSrcEg1), liveSrc[em::kSrcEg1 + e]);
    }
    void drawEf(int e) {
        caption(MX + 126, MY + 44, "LISTENS TO");
        const float v = liveSrc[em::kSrcEf1 + e];
        displayBox(MX + 16, MY + 222, 420, 16); beginPath(); rect(MX + 18, MY + 224, 416 * clampf(v, 0.f, 1.f), 12); fillColor(srcColor(em::kSrcEf1)); fill();
        caption(MX + 16, MY + 250, "FOLLOWER LEVEL", ALIGN_LEFT);
    }
    void drawXy(int x) {
        displayBox(XY_X, XY_Y, XY_W, XY_H);
        beginPath(); moveTo(XY_X + XY_W / 2, XY_Y + 6); lineTo(XY_X + XY_W / 2, XY_Y + XY_H - 6); moveTo(XY_X + 6, XY_Y + XY_H / 2); lineTo(XY_X + XY_W - 6, XY_Y + XY_H / 2);
        strokeColor(Color(255, 255, 255, 10 / 255.f)); stroke();
        const float px = XY_X + 8 + (XY_W - 16) * val[xyp(x, 0)], py = XY_Y + XY_H - 8 - (XY_H - 16) * val[xyp(x, 1)];
        const Color col = srcColor(em::kSrcXy1X);
        beginPath(); moveTo(px, XY_Y + 4); lineTo(px, XY_Y + XY_H - 4); moveTo(XY_X + 4, py); lineTo(XY_X + XY_W - 4, py); strokeColor(withAlpha(col, 0.3f)); stroke();
        beginPath(); circle(px, py, dragXy ? 10 : 8); fillColor(col); fill();
        char b[32]; snprintf(b, sizeof b, "X %.0f%%   Y %.0f%%", val[xyp(x, 0)] * 100, val[xyp(x, 1)] * 100);
        fontSize(9); textLetterSpacing(0.6f); textAlign(ALIGN_LEFT | ALIGN_TOP); fillColor(C_DIM); text(XY_X + XY_W + 16, XY_Y + 100, b, nullptr);
    }
    void drawMidi() {
        static const char* const tip[5] = { "last note velocity", "last note, around middle C", "mod wheel (CC1)", "pitch bend wheel", "channel or poly aftertouch" };
        for (int i = 0; i < 5; ++i) { fontSize(9); textLetterSpacing(0.4f); textAlign(ALIGN_LEFT | ALIGN_MIDDLE); fillColor(C_FAINT); text(MX + 24 + CHIP_W + 16, MY + 52 + i * 40 + CHIP_H / 2, tip[i], nullptr); }
    }
    void drawClassic() {
        static const char* const t[3] = { "LFO 1", "LFO 2", "ENVELOPE" };
        for (int c = 0; c < 3; ++c) {
            const float x0 = MX + 14 + c * CL_COLW;
            if (c) { beginPath(); moveTo(x0 - 6, MY + 38); lineTo(x0 - 6, MY + MH - 12); strokeColor(C_LINE); strokeWidth(1); stroke(); }
            fontSize(8.5f); textLetterSpacing(1.8f); textAlign(ALIGN_LEFT | ALIGN_MIDDLE); fillColor(C_DIM); text(x0 + 2, MY + 49, t[c], nullptr);
        }
        for (int l = 0; l < 2; ++l) {
            const float x0 = MX + 14 + l * CL_COLW, x = x0 + 2, y = MY + 124, w = 172, h = 40;
            displayBox(x, y, w, h);
            const uint32_t b = kL1Wave + 6 * l; const int wave = (int)(val[b] + .5f);
            beginPath();
            for (int i = 0; i <= 120; ++i) { const float tt = (float)i / 120, ph = std::fmod(tt * 2.f, 1.f);
                const float py = y + h / 2 - Engine::lfoShape(ph, wave, val[b + 1]) * h * 0.36f;
                if (!i) moveTo(x + 4 + (w - 8) * tt, py); else lineTo(x + 4 + (w - 8) * tt, py); }
            strokeColor(srcColor(kClassicChip)); strokeWidth(1.4f); stroke();
        }
        const float x0 = MX + 14 + 2 * CL_COLW;
        caption(x0 + 44, MY + 66, "TRIGGER");
        drawAdsr(x0 + 2, MY + 110, 172, 54, val[kEgAttack], val[kEgDecay], val[kEgSustain], val[kEgRelease], srcColor(kClassicChip), liveClassic[kSrcEg]);
    }

    // ---- per-source presets and tap patterns ---------------------------------------
    int compKind() const { return tab <= 3 ? kCompXlfo : (tab <= 5 ? kCompEg : kCompEf); }
    int compIndex() const { return tab <= 3 ? tab : (tab <= 5 ? tab - 4 : tab - 6); }
    bool compHit(float x, float y) const { return tab <= 7 && inRect(x, y, COMP_X, COMP_Y, COMP_W, COMP_H); }
    bool patHit(float x, float y) const { return tapView && inRect(x, y, PAT_X, PAT_Y, PAT_W, PAT_H); }
    struct CompItem { int type, idx; std::string name; };       // type 0 factory, 1 user file, 2 save
    std::vector<CompItem> compItems; int compPendingDel = -1;
    int savingKind = 0;                                         // 0 preset, 1 source preset, 2 tap pattern
    int savingCompKind = 0, savingCompIdx = 0;
    void openListPopup(int kind, float ax, float ay, float aw) {
        compItems.clear();
        if (kind == 3) { const int k = compKind();
            for (int i : compFactoryFor(k)) compItems.push_back({ 0, i, compFactory()[i].name });
            const std::vector<std::string> u = listCompPresets(k); for (int i = 0; i < (int)u.size(); ++i) compItems.push_back({ 1, i, u[i] }); }
        else { for (int i = 0; i < (int)tapFactory().size(); ++i) compItems.push_back({ 0, i, tapFactory()[i].name });
            const std::vector<std::string> u = listTapPatterns(); for (int i = 0; i < (int)u.size(); ++i) compItems.push_back({ 1, i, u[i] }); }
        compItems.push_back({ 2, 0, "Save current..." });
        compPendingDel = -1; popup.open = true; popup.kind = kind; popup.slot = -1; popup.x = ax + aw - 186; popup.y = ay + COMP_H + 4;
        const float ph = compItems.size() * 20.f + 12; if (popup.y + ph > H - 8) popup.y = ay - ph - 4;
        popup.hover = -1; repaint();
    }
    void applyComp(const float* fields) {
        const int k = compKind(), ix = compIndex();
        beginGesture(); for (int f = 0; f < compFieldCount(k); ++f) commit(compParam(k, ix, f), fields[f]); endGesture();
    }
    void applyTaps(const float* fields) { beginGesture(); for (int i = 0; i < kTapPatternFields; ++i) commit(tapPatternParam(i), fields[i]); endGesture(); }
    void pickListItem(int i) {
        const CompItem it = compItems[i];
        if (popup.kind == 3) {
            const int k = compKind(); float fv[kXlfoFields];
            if (it.type == 0) { compFactoryValues(it.idx, fv); applyComp(fv); flash(std::string(compKindName(k)) + " preset: " + it.name); }
            else if (it.type == 1) {
                if (compPendingDel == i) { deleteCompPreset(k, it.name); openListPopup(3, COMP_X, COMP_Y, COMP_W); return; }
                if (loadCompPreset(k, it.name, fv)) { applyComp(fv); flash(std::string(compKindName(k)) + " preset: " + it.name); }
            } else { savingKind = 1; savingCompKind = k; savingCompIdx = compIndex(); saving = true; saveError = false; saveName.clear(); }
        } else {
            float fv[kTapPatternFields];
            if (it.type == 0) { applyTaps(tapFactory()[it.idx].v); flash(std::string("Tap pattern: ") + it.name); }
            else if (it.type == 1) {
                if (compPendingDel == i) { deleteTapPattern(it.name); openListPopup(6, PAT_X, PAT_Y, PAT_W); return; }
                if (loadTapPattern(it.name, fv)) { applyTaps(fv); flash(std::string("Tap pattern: ") + it.name); }
            } else { savingKind = 2; saving = true; saveError = false; saveName.clear(); }
        }
        popup.open = false; repaint();
    }

    // ---- modulation rows: v2 slots (id 0..49) and classic slots (id 100 + slot) ------------
    static bool isClassicRow(int id) { return id >= 100; }
    bool classicUsed(int s) const { return val[slotParam(s, kSlotSrc)] > 0.5f || val[slotParam(s, kSlotDst)] > 0.5f; }
    bool slotUsed(int s) const { return (int)std::round(val[sp(s, kSlotSrc)]) > 0 || (int)std::round(val[sp(s, kSlotDst)]) > 0; }
    uint32_t rowParam(int id, int field) const { return isClassicRow(id) ? slotParam(id - 100, field) : sp(id, field); }
    int rowSrcRaw(int id) const { return (int)std::round(val[rowParam(id, kSlotSrc)]); }
    Color rowColor(int id) const {
        const int s = rowSrcRaw(id);
        if (isClassicRow(id)) return s <= 0 ? C_DIM : (s <= kSrcEg ? srcColor(kClassicChip) : srcColor(em::kSrcVelocity));
        return s <= 0 ? C_DIM : srcColor(s);
    }
    const char* rowSrcShort(int id) const { const int s = rowSrcRaw(id); return isClassicRow(id) ? classicSrcShort(s) : (s > 0 ? em::sourceShort(s) : "-"); }
    const char* rowSrcName(int id) const { const int s = rowSrcRaw(id); return isClassicRow(id) ? sourceName(s) : em::sourceName(s); }
    std::string rowDestName(int id) const { const int d = (int)std::round(val[rowParam(id, kSlotDst)]); return isClassicRow(id) ? destName(d) : destName2(d); }
    bool rowBypassed(int id) const { return val[rowParam(id, kSlotBypass)] > 0.5f; }
    std::vector<int> usedRows() const {
        std::vector<int> v; for (int s = 0; s < kNumSlots2; ++s) if (slotUsed(s)) v.push_back(s);
        for (int s = 0; s < kNumSlots; ++s) if (classicUsed(s)) v.push_back(100 + s);
        return v;
    }
    struct SlotRow { int slot; float x, y; };
    std::vector<SlotRow> slotRows() const {
        std::vector<SlotRow> r; const std::vector<int> u = usedRows();
        for (int i = slotScroll; i < (int)u.size() && (int)r.size() < SLOT_ROWS; ++i) r.push_back({ u[i], SX + 10, SLOT_Y0 + (i - slotScroll) * SLOT_ROWH });
        return r;
    }
    int maxScroll() const { return std::max(0, (int)usedRows().size() - SLOT_ROWS); }
    void drawRowControls(int id, float x, float cy, float kx, bool showDst, int hoverPart) {
        const bool byp = rowBypassed(id);
        beginPath(); circle(x + 10, cy, 3); fillColor(byp ? C_FAINT : rowColor(id)); fill();
        fontSize(8.5f); textLetterSpacing(0.6f); textAlign(ALIGN_LEFT | ALIGN_MIDDLE);
        fillColor(hoverPart == id * 10 + 1 ? C_TEXT : (byp ? C_FAINT : rowColor(id))); text(x + 18, cy + 0.5f, showDst ? rowSrcShort(id) : rowSrcName(id), nullptr);
        if (showDst) {
            fillColor(C_FAINT); text(x + 62, cy + 0.5f, ">", nullptr);
            std::string nm = rowDestName(id); if (nm.size() > 21) nm = nm.substr(0, 20) + ".";
            fillColor(hoverPart == id * 10 + 2 ? C_TEXT : (byp ? C_FAINT : C_DIM)); text(x + 72, cy + 0.5f, nm.c_str(), nullptr);
        }
        const uint32_t ap = rowParam(id, kSlotAmt);
        drawKnobBody(kx, cy, 7.5f, toNorm(ap, val[ap]), true, hoverPart == id * 10 + 3 || activeRow == id, byp, ap);
        char a[16]; fmt(ap, a, sizeof a); fontSize(8); textAlign(ALIGN_LEFT | ALIGN_MIDDLE); fillColor(byp ? C_FAINT : C_DIM); text(kx + 11, cy + 0.5f, a, nullptr);
    }
    void drawRowEnd(int id, float bx, float dx, float cy, int hoverPart) {
        const bool byp = rowBypassed(id);
        beginPath(); circle(bx, cy, 4.5f); fillColor(byp ? C_PANEL : withAlpha(C_ACC, 0.8f)); fill(); strokeColor(byp ? C_FAINT : C_ACC); strokeWidth(1); stroke();
        beginPath(); moveTo(dx - 3.5f, cy - 3.5f); lineTo(dx + 3.5f, cy + 3.5f); moveTo(dx + 3.5f, cy - 3.5f); lineTo(dx - 3.5f, cy + 3.5f);
        strokeColor(hoverPart == id * 10 + 5 ? C_TEXT : C_FAINT); strokeWidth(1.3f); stroke();
    }
    void drawSlots() {
        panel(SX, SY, SW, SH);
        const std::vector<int> rows = usedRows(); int n2 = 0; for (int id : rows) n2 += !isClassicRow(id);
        char b[64]; snprintf(b, sizeof b, "MODULATION  %d / %d", n2, kNumSlots2);
        if ((int)rows.size() > n2) snprintf(b, sizeof b, "MODULATION  %d / %d  +  %d CLASSIC", n2, kNumSlots2, (int)rows.size() - n2);
        fontSize(8); textLetterSpacing(1.8f); textAlign(ALIGN_LEFT | ALIGN_MIDDLE); fillColor(C_FAINT); text(SX + 14, SY + 18, b, nullptr);
        if (rows.empty()) { fontSize(10.5f); textLetterSpacing(0.3f); textAlign(ALIGN_CENTER | ALIGN_MIDDLE); fillColor(C_FAINT);
            text(SX + SW / 2, SY + SH / 2 - 8, "Drag a source chip onto any knob", nullptr);
            fontSize(9); text(SX + SW / 2, SY + SH / 2 + 10, "Each connection appears here", nullptr); return; }
        for (const SlotRow& r : slotRows()) {
            const int s = r.slot; const bool sel = s == selSlot;
            beginPath(); roundedRect(r.x, r.y + 1, SLOT_COLW, SLOT_ROWH - 2, 4); fillColor(sel ? withAlpha(C_ACC, 0.12f) : (isClassicRow(s) ? C_PANEL_LO : C_PANEL_HI)); fill();
            const float cy = r.y + SLOT_ROWH / 2;
            drawRowControls(s, r.x, cy, r.x + 222, true, hoverSlotPart);
            if (isClassicRow(s)) { fontSize(6.5f); textLetterSpacing(0.8f); textAlign(ALIGN_CENTER | ALIGN_MIDDLE); fillColor(C_FAINT); text(r.x + SLOT_COLW - 44, cy + 0.5f, "2.X", nullptr); }
            drawRowEnd(s, r.x + SLOT_COLW - 24, r.x + SLOT_COLW - 10, cy, hoverSlotPart);
        }
        if (maxScroll() > 0) { fontSize(8); textLetterSpacing(1); textAlign(ALIGN_RIGHT | ALIGN_MIDDLE); fillColor(C_FAINT); text(SX + SW - 14, SY + 18, "SCROLL", nullptr); }
    }
    // row part under the pointer: id*10 + part (1 source, 2 destination, 3 amount, 4 bypass, 5 delete), or -1
    int slotPartHit(float x, float y) const {
        for (const SlotRow& r : slotRows()) {
            if (y < r.y || y >= r.y + SLOT_ROWH || x < r.x || x >= r.x + SLOT_COLW) continue;
            const float rx = x - r.x;
            if (rx < 60) return r.slot * 10 + 1;
            if (rx < 210) return r.slot * 10 + 2;
            if (rx < SLOT_COLW - 32) return r.slot * 10 + 3;
            if (rx < SLOT_COLW - 17) return r.slot * 10 + 4;
            return r.slot * 10 + 5;
        }
        return -1;
    }
    void clearRow(int id) { for (int f = 0; f < 4; ++f) commit(rowParam(id, f), 0); }
    void deleteRow(int id) { beginGesture(); clearRow(id); endGesture(); if (selSlot == id) selSlot = -1; slotScroll = std::min(slotScroll, maxScroll()); }
    bool rowClick(int part, float x, float y, int mods) {
        const int id = part / 10, k = part % 10; selSlot = id;
        if (k == 1 || k == 2) { popup.open = true; popup.kind = isClassicRow(id) ? k + 3 : k; popup.slot = id; popup.x = x - 40; popup.y = y + 12; popup.hover = -1; repaint(); return true; }
        if (k == 3) { const uint32_t ap = rowParam(id, kSlotAmt);
            if (mods & (kModifierControl | kModifierSuper)) { beginGesture(); commit(ap, 0.f); endGesture(); return true; }
            beginGesture(); activeRow = id; dragY = y; dragNorm = toNorm(ap, val[ap]); editParameter(ap, true); repaint(); return true; }
        if (k == 4) { const uint32_t bp = rowParam(id, kSlotBypass); beginGesture(); commit(bp, val[bp] > 0.5f ? 0.f : 1.f); endGesture(); return true; }
        deleteRow(id); return true;
    }

    // ---- floating panel: every row that modulates one knob ----------------------------
    struct KnobPanel { bool open = false; uint32_t param = 0; float x = 0, y = 0; } kpan; int hoverKp = -1;
    static constexpr float KP_W = 270, KP_HEAD = 26, KP_ROWH = 22;
    std::vector<int> rowsFor(uint32_t p) const {
        std::vector<int> v;
        const int d2 = destForParam2(p);
        if (d2 > 0) for (int k = 0; k < kNumSlots2; ++k) if ((int)std::round(val[sp(k, kSlotDst)]) == d2 && (int)std::round(val[sp(k, kSlotSrc)]) > 0) v.push_back(k);
        const int d = destForParam(p);
        if (d > 0) for (int k = 0; k < kNumSlots; ++k) if ((int)std::round(val[slotParam(k, kSlotDst)]) == d && (int)std::round(val[slotParam(k, kSlotSrc)]) > 0) v.push_back(100 + k);
        return v;
    }
    void kpLayout(float& x, float& y, float& h) const {
        h = KP_HEAD + std::max<size_t>(1, rowsFor(kpan.param).size()) * KP_ROWH + 8;
        x = std::min(W - KP_W - 8, std::max(8.f, kpan.x)); y = std::min(H - h - 8, std::max(8.f, kpan.y));
    }
    bool kpInside(float px, float py) const { float x, y, h; kpLayout(x, y, h); return px >= x && px < x + KP_W && py >= y && py < y + h; }
    int kpHit(float px, float py) const {
        float x, y, h; kpLayout(x, y, h); const std::vector<int> v = rowsFor(kpan.param);
        for (int i = 0; i < (int)v.size(); ++i) { const float ry = y + KP_HEAD + i * KP_ROWH; if (py < ry || py >= ry + KP_ROWH || px < x || px >= x + KP_W) continue;
            const float rx = px - x; return v[i] * 10 + (rx < 124 ? 1 : (rx < KP_W - 44 ? 3 : (rx < KP_W - 24 ? 4 : 5))); }
        return -1;
    }
    void openKnobPanel(uint32_t p, float px, float py) {
        if (destForParam2(p) <= 0 && destForParam(p) <= 0) return;
        if (rowsFor(p).empty()) { flash("Nothing modulates this control yet. Drag a source chip onto it."); return; }
        kpan.open = true; kpan.param = p; kpan.x = px + 14; kpan.y = py - 10; hoverKp = -1; repaint();
    }
    void drawKnobPanel() {
        float x, y, h; kpLayout(x, y, h); const std::vector<int> v = rowsFor(kpan.param);
        beginPath(); rect(x - 10, y - 4, KP_W + 20, h + 18); fillPaint(boxGradient(x, y + 2, KP_W, h, 6, 12, Color(0, 0, 0, 0.6f), Color(0, 0, 0, 0.f))); fill();
        beginPath(); roundedRect(x, y, KP_W, h, 6); fillColor(C_PANEL_HI); fill(); strokeColor(C_LINE); strokeWidth(1); stroke();
        std::string t = paramDef(kpan.param).name; for (char& ch : t) ch = (char)std::toupper((unsigned char)ch);
        fontSize(8); textLetterSpacing(1.4f); textAlign(ALIGN_LEFT | ALIGN_MIDDLE); fillColor(C_ACC); text(x + 12, y + 14, t.c_str(), nullptr);
        for (int i = 0; i < (int)v.size(); ++i) {
            const int id = v[i]; const float ry = y + KP_HEAD + i * KP_ROWH, cy = ry + KP_ROWH / 2;
            if (hoverKp / 10 == id) { beginPath(); roundedRect(x + 4, ry + 1, KP_W - 8, KP_ROWH - 2, 3); fillColor(withAlpha(C_ACC, 0.10f)); fill(); }
            drawRowControls(id, x + 4, cy, x + 136, false, hoverKp);
            drawRowEnd(id, x + KP_W - 34, x + KP_W - 14, cy, hoverKp);
        }
    }
    bool ringHit(const Ctl& c, float x, float y) const {
        if (c.type != Knob || c.r <= 9) return false;
        if (rowsFor(effParam(c)).empty()) return false;
        const float tw = c.r > 30 ? 4.f : (c.r > 12 ? 3.f : 2.5f), rr = c.r + tw / 2 + 3;
        const float dist = std::sqrt((x - c.x) * (x - c.x) + (y - c.y) * (y - c.y));
        return std::fabs(dist - rr) <= 3.f;
    }

    // ---- knobs ------------------------------------------------------------------------
    bool modInfo(uint32_t p, float& depth, Color& col, float& live) const {
        depth = 0; float best = 0; Color bc = C_DIM; live = 0;
        const int d2 = destForParam2(p);
        if (d2 > 0) for (int s = 0; s < kNumSlots2; ++s) {
            if (val[sp(s, kSlotBypass)] > 0.5f || (int)std::round(val[sp(s, kSlotDst)]) != d2) continue;
            const int src = (int)std::round(val[sp(s, kSlotSrc)]); if (src <= 0) continue;
            const float a = std::fabs(val[sp(s, kSlotAmt)]); depth += a; if (a >= best) { best = a; bc = srcColor(src); }
        }
        const int d = destForParam(p);
        if (d > 0) for (int s = 0; s < kNumSlots; ++s) {
            if (val[slotParam(s, kSlotBypass)] > 0.5f || (int)std::round(val[slotParam(s, kSlotDst)]) != d) continue;
            if ((int)std::round(val[slotParam(s, kSlotSrc)]) <= 0) continue;
            const float a = std::fabs(val[slotParam(s, kSlotAmt)]); depth += a; if (a >= best) { best = a; bc = rowColor(100 + s); }
        }
        if (depth <= 0) return false;
        col = bc; if (d2 > 0 && d2 >= kDstSlotFirst) live = liveOff[d2]; return true;
    }
    void drawKnobBody(float x, float y, float r, float n, bool bipolar, bool hot, bool dim, uint32_t p) {
        const float a0 = 0.75f * 3.14159265f, a1 = 2.25f * 3.14159265f, ang = a0 + (a1 - a0) * n;
        const float tw = r > 20 ? 4.f : (r > 12 ? 3.f : 2.5f);
        beginPath(); arc(x, y, r, a0, a1, NanoVG::CW); strokeColor(C_LINE); strokeWidth(tw); lineCap(ROUND); stroke();
        const float from = bipolar ? (a0 + a1) / 2 : a0;
        Color arcCol = C_ACC;
        if (p == kTimeR || p == kDivR || p == kOffR || p == kPanR || p == kFbR || p == kXfRL) arcCol = C_RIGHT;
        for (int f = 0; f < kNumFilters; ++f) for (int k = 0; k <= kFGain; ++k) if (fpar(f, k) == p) arcCol = filterColor(f);
        if (std::fabs(ang - from) > 0.001f) { beginPath(); arc(x, y, r, std::min(from, ang), std::max(from, ang), NanoVG::CW); strokeColor(dim ? C_FAINT : arcCol); stroke(); }
        float depth, live; Color mc;
        if (r > 9 && modInfo(p, depth, mc, live)) {
            const float span = std::max(0.08f, std::min(1.f, depth * 2.f)) * (a1 - a0) * 0.5f;
            beginPath(); arc(x, y, r + tw / 2 + 3, std::max(a0, ang - span), std::min(a1, ang + span), NanoVG::CW); strokeColor(withAlpha(mc, 0.85f)); strokeWidth(2.f); lineCap(ROUND); stroke();
        }
        const float cr = r - (r > 20 ? 8 : (r > 12 ? 6 : 4));
        beginPath(); circle(x, y, cr);
        fillPaint(radialGradient(x - cr * 0.3f, y - cr * 0.4f, 0, cr * 1.4f, hot ? Color(0x40, 0x3a, 0x34) : C_PANEL_HI, Color(0x15, 0x14, 0x12)));
        fill(); strokeColor(Color(255, 255, 255, 18 / 255.f)); strokeWidth(1); stroke();
        beginPath(); moveTo(x + std::cos(ang) * cr * 0.3f, y + std::sin(ang) * cr * 0.3f); lineTo(x + std::cos(ang) * cr * 0.9f, y + std::sin(ang) * cr * 0.9f);
        strokeColor(dim ? C_DIM : C_TEXT); strokeWidth(r > 12 ? 2.f : 1.5f); lineCap(ROUND); stroke();
    }
    int segCurrent(const Ctl& c, uint32_t p) const {
        if (c.group == GFilt && c.field == kFSlope) { const int v = (int)std::round(val[p]); return v == kSlope6 ? 0 : v + 1; }
        return (int)std::round(val[p]);
    }
    static bool knobBipolar(uint32_t p) { const ParamDef& d = paramDef(p); return (d.hints & kHintBipolar) || (d.min < 0 && std::strcmp(d.unit, "dB") != 0); }
    void learnBadge(float cx, float by, uint32_t p) {
        const int cc = paramCC[p]; const bool waiting = learnWaiting == (int)p; char buf[16];
        snprintf(buf, sizeof buf, waiting ? "?" : "CC%d", cc);
        fontSize(8); textLetterSpacing(0.5f); textAlign(ALIGN_CENTER | ALIGN_MIDDLE);
        const float bw = waiting ? 14 : 32;
        beginPath(); roundedRect(cx - bw / 2, by - 6, bw, 12, 3); fillColor(waiting ? C_ACC : C_PANEL_HI); fill(); strokeColor(C_ACC); strokeWidth(1); stroke();
        fillColor(waiting ? C_BG : C_ACC); text(cx, by + 0.5f, buf, nullptr);
    }
    void drawCtl(const Ctl& c, bool hot) {
        const uint32_t p = effParam(c); const bool dim = dimmed(p); char buf[48];
        const bool dropTarget = dragSrc > 0 && hot && (dragSrc >= kClassicChip ? destForParam(p) > 0 : destForParam2(p) > 0);
        switch (c.type) {
        case Knob: {
            drawKnobBody(c.x, c.y, c.r, toNorm(p, val[p]), knobBipolar(p), hot, dim, p);
            if (dropTarget) { beginPath(); circle(c.x, c.y, c.r + 9); strokeColor(srcColor(dragSrc)); strokeWidth(2); stroke(); }
            const bool big = c.group == GGlobal && (c.field == (int)kTimeL || c.field == (int)kTimeR);
            if (big) {         // the time knobs always show their value
                fmt(p, buf, sizeof buf); fontSize(10); textLetterSpacing(0.6f); textAlign(ALIGN_CENTER | ALIGN_TOP); fillColor(dim ? C_FAINT : (hot ? C_ACC : C_TEXT)); text(c.x, c.y + c.r + 6, buf, nullptr);
            } else if (c.group == GGlobal && (c.y < R2 || c.field == (int)kInGain || c.field == (int)kMix || c.field == (int)kOutGain || c.field == (int)kWidth || c.field == (int)kWetLevel || c.field == (int)kWetPan) && !(hot && editing < 0 && !learnMode && dragSrc <= 0)) {
                fontSize(8.5f); textLetterSpacing(1.5f); textAlign(ALIGN_CENTER | ALIGN_TOP); fillColor(dim ? C_FAINT : C_DIM); text(c.x, c.y + c.r + 6, c.label, nullptr);
            } else if (c.group == GGlobal && (c.y < R2 || c.field == (int)kInGain || c.field == (int)kMix || c.field == (int)kOutGain || c.field == (int)kWidth || c.field == (int)kWetLevel || c.field == (int)kWetPan)) {
                fmt(p, buf, sizeof buf); fontSize(8.5f); textLetterSpacing(0.6f); textAlign(ALIGN_CENTER | ALIGN_TOP); fillColor(C_ACC); text(c.x, c.y + c.r + 6, buf, nullptr);
            } else {           // label and value
                fontSize(8); textLetterSpacing(1.4f); textAlign(ALIGN_CENTER | ALIGN_TOP); fillColor(dim ? C_FAINT : C_DIM); text(c.x, c.y + c.r + 6, c.label, nullptr);
                fmt(p, buf, sizeof buf); fontSize(9.5f); textLetterSpacing(0.4f); fillColor(dim ? C_FAINT : (hot ? C_ACC : Color(0xd6, 0xce, 0xc2))); text(c.x, c.y + c.r + 17, buf, nullptr);
            }
            break; }
        case Toggle: {
            const bool on = val[p] > 0.5f;
            const float w = 30, h = 16, x = c.x - w / 2, y = c.y - h / 2;
            beginPath(); roundedRect(x, y, w, h, h / 2); fillColor(on && !dim ? withAlpha(C_ACC, 0.25f) : C_PANEL_HI); fill(); strokeColor(on && !dim ? C_ACC : C_LINE); strokeWidth(1); stroke();
            beginPath(); circle(on ? x + w - h / 2 : x + h / 2, c.y, 5); fillColor(dim ? C_FAINT : (on ? C_ACC : C_DIM)); fill();
            fontSize(8); textLetterSpacing(1.4f); textAlign(ALIGN_LEFT | ALIGN_MIDDLE); fillColor(on && !dim ? C_TEXT : (dim ? C_FAINT : C_DIM)); text(c.x + w / 2 + 7, c.y + 0.5f, c.label, nullptr);
            break; }
        case Segment: {
            int n = 0; const char* const* names = segLabels(c.group, c.field, n);
            const int cur = segCurrent(c, p); const float sw = c.r / n, x0 = c.x - c.r / 2;
            const Color on = c.group == GFilt ? filterColor(selF) : C_ACC;
            for (int i = 0; i < n; ++i) {
                const float x = x0 + sw * i;
                beginPath(); roundedRect(x + 1, c.y - SEG_H / 2, sw - 2, SEG_H, 4); fillColor(i == cur ? (dim ? C_FAINT : on) : C_PANEL_HI); fill();
                fontSize(8.5f); textLetterSpacing(1.2f); textAlign(ALIGN_CENTER | ALIGN_MIDDLE); fillColor(i == cur ? C_BG : (dim ? C_FAINT : C_DIM)); text(x + sw / 2, c.y + 0.5f, names[i], nullptr);
            }
            break; }
        case Stepper: {
            const float x = c.x - c.r / 2, y = c.y - 11, w = c.r, h = 22;
            beginPath(); roundedRect(x, y, w, h, 4); fillColor(C_PANEL_HI); fill(); strokeColor(hot ? C_DIM : C_LINE); strokeWidth(1); stroke();
            for (int side = 0; side < 2; ++side) { const float ax = side == 0 ? x + 9 : x + w - 9, d = side == 0 ? -1.f : 1.f;
                beginPath(); moveTo(ax - 2.5f * d, c.y - 4); lineTo(ax + 2.5f * d, c.y); lineTo(ax - 2.5f * d, c.y + 4); closePath(); fillColor(C_DIM); fill(); }
            fmt(p, buf, sizeof buf); std::string up(buf); for (char& ch : up) ch = (char)std::toupper((unsigned char)ch);
            if (c.group == GXlfo && c.field == em::kLSteps) up += " STEPS";
            fontSize(8.5f); textLetterSpacing(0.8f); textAlign(ALIGN_CENTER | ALIGN_MIDDLE); fillColor(C_ACC); text(c.x, c.y + 0.5f, up.c_str(), nullptr);
            break; }
        }
        if (learnMode && c.type != Stepper && (paramCC[p] >= 0 || learnWaiting == (int)p))
            learnBadge(c.x, c.type == Knob ? c.y - c.r - 12 : c.y - SEG_H / 2 - 8, p);
    }

    void drawStatus() {
        std::string msg; char vb[48];
        if (flashing()) msg = statusText;
        else if (learnMode) msg = learnWaiting >= 0 ? std::string("Move a MIDI controller: ") + paramDef(learnWaiting).name : "MIDI Learn: click a control, move a controller. Right-click unassigns.";
        else if (dragSrc > 0) { msg = std::string("Drop ") + chipName(dragSrc) + " onto a knob";
            if (hover >= 0) { const uint32_t p = effParam(controls()[hover]); const bool ok = dragSrc >= kClassicChip ? destForParam(p) > 0 : destForParam2(p) > 0; if (ok) msg = std::string("Drop ") + chipName(dragSrc) + " onto " + paramDef(p).name; } }
        else if (hover >= 0) { const uint32_t p = effParam(controls()[hover]); fmt(p, vb, sizeof vb); msg = std::string(paramDef(p).name) + "   " + vb; }
        else msg = "Drag a source chip onto any knob to modulate it.  Hover for tips.";
        fontSize(9); textLetterSpacing(0.6f); textAlign(ALIGN_LEFT | ALIGN_MIDDLE); fillColor(learnMode || flashing() ? C_ACC : C_FAINT);
        text(18, STATUS_Y, msg.c_str(), nullptr);
        if (learnMode && learnWaiting < 0) {
            const float cx = W - 16 - 110; beginPath(); roundedRect(cx, STATUS_Y - 8, 110, 16, 4); fillColor(hoverClear ? withAlpha(C_ACC, 0.25f) : C_PANEL_HI); fill(); strokeColor(C_ACC); strokeWidth(1); stroke();
            fontSize(8); textLetterSpacing(1.2f); textAlign(ALIGN_CENTER | ALIGN_MIDDLE); fillColor(C_TEXT); text(cx + 55, STATUS_Y + 0.5f, "CLEAR ALL CCs", nullptr);
        }
    }
    void flash(const std::string& s) { statusText = s; flashUntil = nowSec() + 2.0; repaint(); }
    bool flashing() const { return nowSec() < flashUntil; }
    void drawEditBox() {
        const Ctl& c = controls()[editing];
        const float w = 90, h = 22, x = c.x - w / 2, y = c.y + c.r + 4;
        beginPath(); roundedRect(x, y, w, h, 4); fillColor(C_DISPLAY); fill(); strokeColor(C_ACC); strokeWidth(1); stroke();
        fontSize(12); textLetterSpacing(0.5f); textAlign(ALIGN_CENTER | ALIGN_MIDDLE); fillColor(C_TEXT); text(c.x, y + h / 2 + 0.5f, (editBuf + "|").c_str(), nullptr);
    }

    // ---- popups: style 0, v2 source 1, v2 destination 2, source presets 3, classic source 4, classic destination 5, tap patterns 6 ----
    struct Popup { bool open = false; int kind = 0, slot = -1; float x = 0, y = 0; int hover = -1; } popup;
    bool listPopup() const { return popup.kind == 3 || popup.kind == 6; }
    int popupCount() const {
        switch (popup.kind) { case 0: return kNumStyles; case 1: return em::kNumSources - 1; case 2: return kNumDests2 - 1; case 4: return kNumSources - 1; case 5: return kNumDests - 1; default: return (int)compItems.size(); }
    }
    std::string popupLabel(int i) const {
        if (listPopup()) { const CompItem& it = compItems[i]; return it.type == 1 ? (compPendingDel == i ? "Delete? click again" : "  " + it.name) : it.name; }
        switch (popup.kind) { case 0: return styleName(i); case 1: return em::sourceName(i + 1); case 2: return destName2(i + 1); case 4: return sourceName(i + 1); default: return destName(i + 1); }
    }
    float popupRowH() const { return popup.kind == 0 ? 22.f : 20.f; }
    void popupLayout(float& x, float& y, float& w, float& h, int& rows, float& colw) const {
        const int n = popupCount();
        if (popup.kind == 0) { rows = n; colw = 160; }
        else if (listPopup()) { rows = n; colw = 180; }
        else if (popup.kind == 1) { rows = 9; colw = 110; }
        else if (popup.kind == 4) { rows = 8; colw = 110; }
        else { rows = popup.kind == 2 ? 28 : 25; colw = 150; }
        const int cols = (n + rows - 1) / rows; w = cols * colw + 12; h = rows * popupRowH() + 12;
        x = std::min(W - w - 8, std::max(8.f, popup.x)); y = std::min(H - h - 8, std::max(8.f, popup.y));
    }
    void popupCell(int i, float x, float y, float colw, int rows, float& ix, float& iy) const { ix = x + 6 + (i / rows) * colw; iy = y + 6 + (i % rows) * popupRowH(); }
    int popupCurrent() const {
        if (popup.kind == 0) return (int)std::round(val[fpar(selF, kFStyle)]);
        if (popup.slot < 0 || listPopup()) return -1;
        return (int)std::round(val[rowParam(popup.slot, (popup.kind == 1 || popup.kind == 4) ? kSlotSrc : kSlotDst)]) - 1;
    }
    void drawPopup() {
        float x, y, w, h, colw; int rows; popupLayout(x, y, w, h, rows, colw);
        beginPath(); rect(x - 10, y - 4, w + 20, h + 18); fillPaint(boxGradient(x, y + 2, w, h, 6, 12, Color(0, 0, 0, 0.6f), Color(0, 0, 0, 0.f))); fill();
        beginPath(); roundedRect(x, y, w, h, 6); fillColor(C_PANEL_HI); fill(); strokeColor(C_LINE); strokeWidth(1); stroke();
        const int cur = popupCurrent(); const float rh = popupRowH();
        for (int i = 0; i < popupCount(); ++i) {
            float ix, iy; popupCell(i, x, y, colw, rows, ix, iy);
            if (i == popup.hover) { beginPath(); roundedRect(ix, iy, colw - 4, rh, 3); fillColor(withAlpha(C_ACC, 0.22f)); fill(); }
            if (popup.kind == 1) { beginPath(); circle(ix + 7, iy + rh / 2, 3); fillColor(srcColor(i + 1)); fill(); }
            if (popup.kind == 4) { beginPath(); circle(ix + 7, iy + rh / 2, 3); fillColor(i < 3 ? srcColor(kClassicChip) : srcColor(em::kSrcVelocity)); fill(); }
            if (popup.kind == 0 && i == kNumStyles2) { beginPath(); rect(ix + 4, iy - 0.5f, colw - 12, 1); fillColor(C_LINE); fill(); }
            if (listPopup() && i > 0 && compItems[i].type != compItems[i - 1].type) { beginPath(); rect(ix + 4, iy - 0.5f, colw - 12, 1); fillColor(C_LINE); fill(); }
            const bool accent = listPopup() && (compItems[i].type == 2 || compPendingDel == i);
            fontSize(popup.kind == 0 ? 10.5f : 9.5f); textLetterSpacing(0.3f); textAlign(ALIGN_LEFT | ALIGN_MIDDLE); fillColor(i == cur || accent ? C_ACC : (i == popup.hover ? C_TEXT : C_DIM));
            text(ix + ((popup.kind == 1 || popup.kind == 4) ? 15 : 6), iy + rh / 2 + 0.5f, popupLabel(i).c_str(), nullptr);
        }
    }
    int popupHit(float px, float py) const {
        float x, y, w, h, colw; int rows; popupLayout(x, y, w, h, rows, colw);
        for (int i = 0; i < popupCount(); ++i) { float ix, iy; popupCell(i, x, y, colw, rows, ix, iy); if (px >= ix && px < ix + colw - 4 && py >= iy && py < iy + popupRowH()) return i; }
        return -1;
    }
    bool popupInside(float px, float py) const { float x, y, w, h, colw; int rows; popupLayout(x, y, w, h, rows, colw); return px >= x && px < x + w && py >= y && py < y + h; }
    void popupPick(int i) {
        if (listPopup()) { pickListItem(i); return; }
        beginGesture();
        if (popup.kind == 0) commit(fpar(selF, kFStyle), (float)i);
        else if (popup.slot >= 0) commit(rowParam(popup.slot, (popup.kind == 1 || popup.kind == 4) ? kSlotSrc : kSlotDst), (float)(i + 1));
        endGesture(); popup.open = false; repaint();
    }

    // ---- presets ------------------------------------------------------------------
    bool matchesValues(const float* v) const {
        for (uint32_t i = 0; i < kParamCount; ++i) {
            if (!isSetting(i) || i == kLockMix || (i == kMix && val[kLockMix] > 0.5f)) continue;
            if (std::fabs(val[i] - v[i]) > 1e-4f * std::max(1.f, std::fabs(v[i]))) return false;
        }
        return true;
    }
    void applyValues(const float* v) {
        float w[kParamCount]; std::copy(v, v + kParamCount, w);
        if (val[kLockMix] > 0.5f) { w[kMix] = val[kMix]; w[kLockMix] = 1; }       // LOCK MIX keeps the mix
        beginGesture(); applyAll(w); endGesture();
    }
    void selectFirstOn() { selF = 0; for (int f = 0; f < kNumFilters; ++f) if (fOn(f)) { selF = f; break; } }
    void applyFactory(int idx) { idx = (idx % (int)kNumPresets + (int)kNumPresets) % (int)kNumPresets; applyValues(presets()[idx].v); curFactory = idx; curUser.clear(); selectFirstOn(); repaint(); }
    void applyUser(const std::string& name) { float v[kParamCount]; std::copy(defv, defv + kParamCount, v); if (!loadUserPreset(name, v)) return; applyValues(v); std::copy(v, v + kParamCount, userVals); curUser = name; curFactory = -1; selectFirstOn(); repaint(); }
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
        if (val[kLockMix] > 0.5f) { fontSize(6.5f); textLetterSpacing(1.f); textAlign(ALIGN_LEFT | ALIGN_MIDDLE); fillColor(C_FAINT); text(x + a + 6, y + h / 2 + 0.5f, "MIX LOCKED", nullptr); }
    }
    // preset browser: categories, user presets and favourites in columns; typing searches names and categories.
    // kind: 0 factory, 1 user, 2 save button, 3 search field, 4 favourite (idx into favs)
    struct MRect { float x, y, w, h; int kind; int idx; };
    static std::string lower(const std::string& s) { std::string r(s); for (char& c : r) c = (char)std::tolower((unsigned char)c); return r; }
    std::string favKey(int kind, int idx) const { return kind == 0 ? std::string("f:") + presets()[idx].name : (kind == 1 ? "u:" + userPresets[idx] : (idx < (int)favs.size() ? favs[idx] : std::string())); }
    bool isFav(const std::string& k) const { return std::find(favs.begin(), favs.end(), k) != favs.end(); }
    void toggleFav(const std::string& k) {
        if (k.empty()) return;
        auto it = std::find(favs.begin(), favs.end(), k); if (it != favs.end()) favs.erase(it); else favs.push_back(k);
        saveFavourites(favs); repaint();
    }
    static constexpr int kMenuCols = kNumCategories + 2, kMenuRows = 16;
    void menuLayout(float& mx, float& my, float& mw, float& mh, std::vector<MRect>& items) const {
        mw = PM_COLW * kMenuCols + 2 * PM_PAD; mx = std::min(W - mw - 8, std::max(8.f, PS_X + PS_W / 2 - mw / 2)); my = PS_Y + PS_H + 4;
        items.clear();
        const float top = my + PM_PAD + PM_ROW + 6 + PM_HEAD;
        int maxRows = 1;
        items.push_back({ mx + PM_PAD + PM_COLW, my + PM_PAD, 300, PM_ROW, 3, 0 });
        items.push_back({ mx + mw - PM_PAD - 150, my + PM_PAD, 150, PM_ROW, 2, 0 });
        if (!query.empty()) {
            const std::string q = lower(query); int k = 0;
            auto place = [&](int kind, int idx) { const int col = k / kMenuRows, row = k % kMenuRows; if (col >= kMenuCols) return; items.push_back({ mx + PM_PAD + col * PM_COLW, top + row * PM_ROW, PM_COLW - 4, PM_ROW, kind, idx }); maxRows = std::max(maxRows, row + 1); ++k; };
            for (uint32_t i = 0; i < kNumPresets; ++i) if (lower(presets()[i].name).find(q) != std::string::npos || lower(presets()[i].category).find(q) != std::string::npos) place(0, (int)i);
            for (int u = 0; u < (int)userPresets.size(); ++u) if (lower(userPresets[u]).find(q) != std::string::npos || std::string("user").find(q) != std::string::npos) place(1, u);
        } else {
            int rows[kNumCategories] = {};
            for (uint32_t i = 0; i < kNumPresets; ++i) {
                const char* cat = presets()[i].category;
                if (std::strcmp(cat, "Init") == 0) { items.push_back({ mx + PM_PAD, my + PM_PAD, PM_COLW - 4, PM_ROW, 0, (int)i }); continue; }
                for (int c = 0; c < kNumCategories; ++c)
                    if (std::strcmp(cat, kCategories[c]) == 0) { items.push_back({ mx + PM_PAD + c * PM_COLW, top + rows[c] * PM_ROW, PM_COLW - 4, PM_ROW, 0, (int)i }); maxRows = std::max(maxRows, ++rows[c]); }
            }
            for (int u = 0; u < (int)userPresets.size() && u < kMenuRows; ++u) items.push_back({ mx + PM_PAD + kNumCategories * PM_COLW, top + u * PM_ROW, PM_COLW - 4, PM_ROW, 1, u });
            maxRows = std::max(maxRows, std::min(kMenuRows, (int)userPresets.size()));
            for (int f = 0; f < (int)favs.size() && f < kMenuRows; ++f) items.push_back({ mx + PM_PAD + (kNumCategories + 1) * PM_COLW, top + f * PM_ROW, PM_COLW - 4, PM_ROW, 4, f });
            maxRows = std::max(maxRows, std::min(kMenuRows, (int)favs.size()));
        }
        mh = PM_PAD + PM_ROW + 6 + PM_HEAD + maxRows * PM_ROW + PM_PAD;
    }
    std::string itemLabel(const MRect& r) const {
        if (r.kind == 0) return presets()[r.idx].name;
        if (r.kind == 1) return userPresets[r.idx];
        if (r.kind == 4 && r.idx < (int)favs.size()) return favs[r.idx].substr(2);
        return "";
    }
    void drawPresetMenu() {
        float mx, my, mw, mh; std::vector<MRect> items; menuLayout(mx, my, mw, mh, items);
        beginPath(); rect(mx - 12, my - 4, mw + 24, mh + 20); fillPaint(boxGradient(mx, my + 2, mw, mh, 6, 14, Color(0, 0, 0, 0.6f), Color(0, 0, 0, 0.f))); fill();
        beginPath(); roundedRect(mx, my, mw, mh, 6); fillColor(C_PANEL_HI); fill(); strokeColor(C_LINE); strokeWidth(1); stroke();
        beginPath(); moveTo(mx + PM_PAD, my + PM_PAD + PM_ROW + 3); lineTo(mx + mw - PM_PAD, my + PM_PAD + PM_ROW + 3); strokeColor(C_LINE); stroke();
        fontSize(8.5f); textLetterSpacing(1.8f); textAlign(ALIGN_LEFT | ALIGN_MIDDLE);
        const float hy = my + PM_PAD + PM_ROW + 6 + PM_HEAD / 2;
        if (query.empty()) {
            for (int c = 0; c < kMenuCols; ++c) {
                std::string up = c < kNumCategories ? kCategories[c] : (c == kNumCategories ? "User" : "Favourites"); for (char& ch : up) ch = (char)std::toupper((unsigned char)ch);
                fillColor(c < kNumCategories ? C_ACC : C_FAV); text(mx + PM_PAD + c * PM_COLW + 12, hy, up.c_str(), nullptr);
            }
            fontSize(10); textLetterSpacing(0.4f); fillColor(C_FAINT);
            if (userPresets.empty()) text(mx + PM_PAD + kNumCategories * PM_COLW + 12, hy + PM_HEAD / 2 + PM_ROW / 2, "(none saved yet)", nullptr);
            if (favs.empty()) text(mx + PM_PAD + (kNumCategories + 1) * PM_COLW + 12, hy + PM_HEAD / 2 + PM_ROW / 2, "(click a star)", nullptr);
        } else {
            int found = 0; for (const MRect& r : items) if (r.kind == 0 || r.kind == 1) ++found;
            char b[64]; snprintf(b, sizeof b, found ? "%d MATCHING PRESET%s" : "NO PRESET MATCHES", found, found == 1 ? "" : "S"); fillColor(C_ACC); text(mx + PM_PAD + 12, hy, b, nullptr);
        }
        for (int i = 0; i < (int)items.size(); ++i) {
            const MRect& r = items[i]; const bool hot = i == menuHover;
            if (r.kind == 2) {
                beginPath(); roundedRect(r.x, r.y + 1, r.w, r.h - 2, 4); fillColor(hot ? withAlpha(C_ACC, 0.25f) : C_PANEL); fill(); strokeColor(C_ACC); strokeWidth(1); stroke();
                fontSize(9.5f); textLetterSpacing(1.6f); textAlign(ALIGN_CENTER | ALIGN_MIDDLE); fillColor(C_TEXT); text(r.x + r.w / 2, r.y + r.h / 2 + 0.5f, "SAVE CURRENT AS...", nullptr);
                continue;
            }
            if (r.kind == 3) {             // search field
                beginPath(); roundedRect(r.x, r.y + 1, r.w, r.h - 2, 4); fillColor(C_DISPLAY); fill(); strokeColor(query.empty() ? C_LINE : C_ACC); strokeWidth(1); stroke();
                beginPath(); circle(r.x + 12, r.y + r.h / 2, 4); moveTo(r.x + 15, r.y + r.h / 2 + 3); lineTo(r.x + 18, r.y + r.h / 2 + 6); strokeColor(C_DIM); strokeWidth(1.3f); stroke();
                fontSize(11); textLetterSpacing(0.3f); textAlign(ALIGN_LEFT | ALIGN_MIDDLE); fillColor(query.empty() ? C_FAINT : C_TEXT);
                text(r.x + 26, r.y + r.h / 2 + 0.5f, query.empty() ? "Type to search names and categories" : (query + "|").c_str(), nullptr);
                continue;
            }
            const std::string key = favKey(r.kind, r.idx);
            const bool isCur = r.kind == 0 ? (curUser.empty() && r.idx == curFactory) : (r.kind == 1 ? curUser == userPresets[r.idx] : false);
            const bool del = r.kind == 1 && pendingDelete == r.idx;
            if (hot || del) { beginPath(); roundedRect(r.x, r.y, r.w, r.h, 4); fillColor(del ? Color(0xd0, 0x50, 0x40, 0.35f) : withAlpha(C_ACC, 0.18f)); fill(); }
            if (isCur) { beginPath(); circle(r.x + 6, r.y + r.h / 2, 2.5f); fillColor(C_ACC); fill(); }
            fontSize(11); textLetterSpacing(0.3f); textAlign(ALIGN_LEFT | ALIGN_MIDDLE); fillColor(hot || isCur || del ? C_TEXT : C_DIM);
            const std::string label = del ? "Delete? click again" : itemLabel(r);
            text(r.x + 12, r.y + r.h / 2 + 0.5f, label.c_str(), nullptr);
            // the favourite star
            const bool fav = isFav(key);
            if (fav || hot) drawStar(r.x + r.w - 10, r.y + r.h / 2, 5.f, fav);
        }
    }
    void drawStar(float cx, float cy, float rr, bool filled) {
        beginPath();
        for (int k = 0; k < 10; ++k) { const float a = -3.14159265f / 2 + k * 3.14159265f / 5, q = k % 2 ? rr * 0.45f : rr; if (!k) moveTo(cx + q * std::cos(a), cy + q * std::sin(a)); else lineTo(cx + q * std::cos(a), cy + q * std::sin(a)); }
        closePath();
        if (filled) { fillColor(C_FAV); fill(); } else { strokeColor(C_DIM); strokeWidth(1); stroke(); }
    }
    int menuHit(float x, float y) const { float mx, my, mw, mh; std::vector<MRect> it; menuLayout(mx, my, mw, mh, it);
        for (int i = 0; i < (int)it.size(); ++i) if (x >= it[i].x && x < it[i].x + it[i].w && y >= it[i].y && y < it[i].y + it[i].h) return i;
        return -1; }
    MRect menuItem(int i) const { float mx, my, mw, mh; std::vector<MRect> it; menuLayout(mx, my, mw, mh, it); return it[i]; }
    void openMenu() { userPresets = listUserPresets(); favs = loadFavourites(); query.clear(); menuOpen = true; menuHover = -1; pendingDelete = -1; repaint(); }
    void applyFavourite(int idx) {
        if (idx >= (int)favs.size()) return;
        const std::string& k = favs[idx];
        if (k.compare(0, 2, "f:") == 0) { for (uint32_t i = 0; i < kNumPresets; ++i) if (k.substr(2) == presets()[i].name) { applyFactory((int)i); return; } }
        else applyUser(k.substr(2));
    }
    void drawSaveBox() {
        const float w = 360, h = 96, x = W / 2 - w / 2, y = 150;
        beginPath(); rect(0, 0, W, H); fillColor(Color(0, 0, 0, 0.45f)); fill();
        beginPath(); roundedRect(x, y, w, h, 8); fillColor(C_PANEL_HI); fill(); strokeColor(C_ACC); strokeWidth(1); stroke();
        const char* t = savingKind == 2 ? "SAVE TAP PATTERN AS" : (savingKind == 1 ? (savingCompKind == kCompXlfo ? "SAVE LFO PRESET AS" : (savingCompKind == kCompEg ? "SAVE ENVELOPE PRESET AS" : "SAVE FOLLOWER PRESET AS")) : "SAVE PRESET AS");
        fontSize(9.5f); textLetterSpacing(2.f); textAlign(ALIGN_LEFT | ALIGN_TOP); fillColor(C_DIM); text(x + 16, y + 14, t, nullptr);
        beginPath(); roundedRect(x + 16, y + 36, w - 32, 26, 4); fillColor(C_DISPLAY); fill(); strokeColor(C_LINE); stroke();
        fontSize(13); textLetterSpacing(0.4f); textAlign(ALIGN_LEFT | ALIGN_MIDDLE); fillColor(C_TEXT); text(x + 26, y + 49.5f, (saveName + "|").c_str(), nullptr);
        fontSize(8.5f); textLetterSpacing(1.2f); fillColor(C_FAINT); textAlign(ALIGN_RIGHT | ALIGN_TOP);
        text(x + w - 16, y + 72, saveError ? "COULD NOT SAVE - CHECK THE NAME" : "ENTER TO SAVE   ·   ESC TO CANCEL", nullptr);
    }

    // ---- tap tempo -------------------------------------------------------------------
    void tapTempo(uint time) {
        const double now = time;
        if (nTaps > 0 && (now - taps[nTaps - 1] > 2500 || now <= taps[nTaps - 1])) nTaps = 0;
        if (nTaps == 5) { for (int i = 0; i < 4; ++i) taps[i] = taps[i + 1]; nTaps = 4; }
        taps[nTaps++] = now;
        tapFlash = 6;
        if (nTaps < 2) { flash("Tap again..."); return; }
        const float ms = clampf((float)((taps[nTaps - 1] - taps[0]) / (nTaps - 1)), kMinDelayMs, kMaxDelayMs);
        const float baseL = val[kSyncL] > 0.5f ? val[kOutTimeL] : val[kTimeL], baseR = val[kSyncR] > 0.5f ? val[kOutTimeR] : val[kTimeR];
        const float ratio = linked() ? 1.f : std::max(baseR, 0.5f) / std::max(baseL, 0.5f);
        beginGesture();
        commit(kSyncL, 0); commit(kTimeL, ms);
        if (!linked()) { commit(kSyncR, 0); commit(kTimeR, clampf(ms * ratio, kMinDelayMs, kMaxDelayMs)); }
        endGesture();
        val[kOutTimeL] = ms; if (!linked()) val[kOutTimeR] = ms * ratio;
        char b[64]; snprintf(b, sizeof b, "Tap tempo: %.0f ms  (%.1f BPM)", ms, 60000.f / ms); flash(b);
    }

    // ---- modulation assign ---------------------------------------------------------
    void assign(int src, uint32_t p) {
        if (src >= kClassicChip) { const int d = destForParam(p); if (d <= 0) { flash("Classic sources reach what Strata 2 could modulate - use an XLFO, EG or EF"); return; } assignClassic(kSrcLfo1 + src - kClassicChip, d); return; }
        const int d = destForParam2(p); if (d <= 0) { flash("That control can't be modulated"); return; }
        assignDst(src, d);
    }
    static float defaultAmount(bool timeDst, bool keySrc, bool freqDst) { return timeDst ? (keySrc ? -1.f : 0.005f) : (keySrc && freqDst ? 0.2f : 0.3f); }
    void assignDst(int src, int dst) {
        int slot = -1; for (int s = 0; s < kNumSlots2; ++s) if (!slotUsed(s)) { slot = s; break; }
        if (slot < 0) { flash("All 50 modulation slots are in use - remove one first"); return; }
        const bool freq = dst == kDstF1Freq || dst == kDstF2Freq || (dst >= kV2F36First && dst < kV2TapFirst && (dst - kV2F36First) % 3 == 0);
        beginGesture();
        commit(sp(slot, kSlotSrc), (float)src); commit(sp(slot, kSlotDst), (float)dst);
        commit(sp(slot, kSlotAmt), defaultAmount(dst == kDstTimeL || dst == kDstTimeR, src == em::kSrcKey, freq)); commit(sp(slot, kSlotBypass), 0);
        endGesture();
        revealRow(slot);
        flash(std::string(em::sourceName(src)) + " > " + destName2(dst));
    }
    void assignClassic(int src, int dst) {
        int slot = -1; for (int s = 0; s < kNumSlots; ++s) if (!classicUsed(s)) { slot = s; break; }
        if (slot < 0) { flash("All 24 classic slots are in use - remove one first"); return; }
        beginGesture();
        commit(slotParam(slot, kSlotSrc), (float)src); commit(slotParam(slot, kSlotDst), (float)dst);
        commit(slotParam(slot, kSlotAmt), defaultAmount(dst == kDstTimeL || dst == kDstTimeR, false, false)); commit(slotParam(slot, kSlotBypass), 0);
        endGesture();
        revealRow(100 + slot);
        flash(std::string("Classic ") + sourceName(src) + " > " + destName(dst));
    }
    void revealRow(int id) {
        selSlot = id; const std::vector<int> u = usedRows();
        for (int i = 0; i < (int)u.size(); ++i) if (u[i] == id) { if (i >= slotScroll + SLOT_ROWS) slotScroll = i - SLOT_ROWS + 1; if (i < slotScroll) slotScroll = i; }
    }

    // ---- hit testing ----------------------------------------------------------------
    static bool inRect(float x, float y, float rx, float ry, float rw, float rh) { return x >= rx && x < rx + rw && y >= ry && y < ry + rh; }
    int hit(float x, float y, int* seg = nullptr) const {
        const std::vector<Ctl>& cs = controls();
        for (int i = 0; i < (int)cs.size(); ++i) {
            const Ctl& c = cs[i]; if (!visible(c)) continue;
            switch (c.type) {
            case Knob: { const float rr = c.r + 6; if ((x - c.x) * (x - c.x) + (y - c.y) * (y - c.y) <= rr * rr) return i; break; }
            case Toggle: if (x >= c.x - 18 && x <= c.x + 64 && std::fabs(y - c.y) <= 10) return i; break;
            case Segment: { int n = 0; segLabels(c.group, c.field, n); const float x0 = c.x - c.r / 2;
                if (x >= x0 && x < x0 + c.r && std::fabs(y - c.y) <= SEG_H / 2 + 1) { if (seg) *seg = (int)((x - x0) / (c.r / n)); return i; } break; }
            case Stepper: if (std::fabs(x - c.x) <= c.r / 2 && std::fabs(y - c.y) <= 11) { if (seg) *seg = x < c.x ? -1 : 1; return i; } break;
            }
        }
        return -1;
    }
    int headerHit(float x, float y) const {
        if (y < PS_Y || y > PS_Y + PS_H) return 0;
        if (x >= LEARN_X && x <= LEARN_X + LEARN_W) return 1;
        if (x >= MIDI_X && x <= MIDI_X + MIDI_W) return 2;
        if (x >= UNDO_X && x <= UNDO_X + UR_W) return 3;
        if (x >= REDO_X && x <= REDO_X + UR_W) return 4;
        if (x >= HELP_X && x <= HELP_X + HELP_W) return 5;
        if (x >= A_X && x < A_X + AB_W) return 6;
        if (x >= A_X + AB_W && x < A_X + 2 * AB_W) return 7;
        if (x >= COPY_X && x <= COPY_X + COPY_W) return 8;
        return 0;
    }
    int presetBarHit(float x, float y) const { if (y < PS_Y || y > PS_Y + PS_H || x < PS_X || x > PS_X + PS_W) return -1; return x < PS_X + PS_ARROW ? 0 : (x > PS_X + PS_W - PS_ARROW ? 2 : 1); }
    int filterTabHit(float x, float y, bool* dot = nullptr) const {
        for (int f = 0; f < kNumFilters; ++f) { const float fx = FT_X + f * (FT_W + 4); if (inRect(x, y, fx, FT_Y, FT_W, FT_H)) { if (dot) *dot = x < fx + 24; return f; } }
        return -1;
    }
    int viewHit(float x, float y) const { if (std::fabs(y - VIEW_Y) > 9 || x < VIEW_X || x >= VIEW_X + VIEW_W) return -1; return (int)((x - VIEW_X) / (VIEW_W / 2)); }
    int tabHit(float x, float y) const { if (std::fabs(y - TAB_Y) > TAB_H / 2) return -1; for (int t = 0; t < kNumTabs; ++t) { const float tx = tabX(t); if (x >= tx && x < tx + TAB_W) return t; } return -1; }
    int chipHit(float x, float y) const { for (const Chip& c : chips()) if (inRect(x, y, c.x, c.y, CHIP_W, CHIP_H)) return c.src; return -1; }
    bool styleHit(float x, float y) const { return inRect(x, y, STYLE_X, STYLE_Y, STYLE_W, 26); }
    bool stepHit(float x, float y) const { return tab <= 3 && val[lp_(tab, em::kLMode)] > 0.5f && inRect(x, y, STEP_X, STEP_Y, STEP_W, STEP_H); }
    bool xyHit(float x, float y) const { return tab >= 8 && tab <= 9 && inRect(x, y, XY_X, XY_Y, XY_W, XY_H); }
    bool clearHit(float x, float y) const { return learnMode && learnWaiting < 0 && inRect(x, y, W - 16 - 110, STATUS_Y - 8, 110, 16); }
    int areaHit(float x, float y) const {
        if (inRect(x, y, TAP_X, TAP_Y, TAP_W, TAP_H)) return kTipTap;
        if (inRect(x, y, CLR2_X, CLR2_Y, CLR2_W, CLR2_H)) return kTipClear;
        if (viewHit(x, y) >= 0) return kTipView;
        if (patHit(x, y)) return kTipTapPatterns;
        if (compHit(x, y)) return kTipCompPresets;
        if (inRect(x, y, DISP_X, DISP_Y, DISP_W, DISP_H)) return tapView ? kTipTapEdit : kTipEchoes;
        if (inRect(x, y, FD_X, FD_Y, FD_W, FD_H)) return kTipFilter;
        if (filterTabHit(x, y) >= 0) return kTipFilterTab;
        if (stepHit(x, y)) return kTipStepEdit;
        if (xyHit(x, y)) return kTipXyPad;
        if (tabHit(x, y) >= 0) return kTipSourceTab;
        if (tab == kTabClassic) { for (int l = 0; l < 2; ++l) if (inRect(x, y, MX + 16 + l * CL_COLW, MY + 124, 172, 40)) return kTipLfo; if (inRect(x, y, MX + 16 + 2 * CL_COLW, MY + 110, 172, 54)) return kTipEnv; }
        return -1;
    }
    void setStepAt(float x, float y, bool reset) {
        const int l = tab, s = (int)clampf((x - STEP_X) / (STEP_W / kNumSteps), 0, kNumSteps - 1);
        const float v = reset ? 0.f : clampf((STEP_Y + STEP_H / 2 - y) / (STEP_H / 2 - 6), -1.f, 1.f);
        setValue(lp_(l, em::kLStep1 + s), std::round(v * 20.f) / 20.f);
    }
    void setXyAt(float x, float y) { const int k = tab - 8; setValue(xyp(k, 0), clampf((x - XY_X - 8) / (XY_W - 16), 0.f, 1.f)); setValue(xyp(k, 1), clampf((XY_Y + XY_H - 8 - y) / (XY_H - 16), 0.f, 1.f)); }

    void setLearn(bool on) { learnMode = on; learnWaiting = -1; if (!on) setState("cmd", "cancel"); repaint(); }
    void learn(uint32_t p) { learnWaiting = (int)p; char b[24]; snprintf(b, sizeof b, "learn:%u", p); setState("cmd", b); repaint(); }
    void endEdit(bool apply) {
        if (editing < 0) return;
        const uint32_t p = effParam(controls()[editing]); float v;
        if (apply && parse(p, editBuf, v)) { beginGesture(); commit(p, v); endGesture(); }
        editing = -1; editBuf.clear(); repaint();
    }
    static float angleNorm(const Ctl& c, float x, float y) {
        const float a = std::atan2(y - c.y, x - c.x), a0 = 0.75f * 3.14159265f;
        float rel = a - a0; while (rel < 0) rel += 2 * 3.14159265f;
        if (rel > 1.5f * 3.14159265f) rel = rel > 1.75f * 3.14159265f ? 0.f : 1.5f * 3.14159265f;
        return rel / (1.5f * 3.14159265f);
    }

    bool onKeyboard(const KeyboardEvent& ev) override {
        if (!ev.press) return false;
        std::string* buf = saving ? &saveName : (editing >= 0 ? &editBuf : nullptr);
        if (!buf && menuOpen) {                              // typing in the preset browser searches
            if (ev.key == kKeyEscape) { if (!query.empty()) query.clear(); else menuOpen = false; }
            else if (ev.key == kKeyBackspace || ev.key == kKeyDelete) { if (!query.empty()) query.pop_back(); }
            else if (ev.key >= 32 && ev.key < 127 && query.size() < 32) query += (char)ev.key;
            else return false;
            menuHover = -1; pendingDelete = -1; repaint(); return true;
        }
        if (!buf) {
            const bool cmd = (ev.mod & (kModifierControl | kModifierSuper)) != 0;
            if (cmd && (ev.key == 'z' || ev.key == 'Z')) { if (ev.mod & kModifierShift) redo(); else undo(); return true; }
            if (cmd && (ev.key == 'y' || ev.key == 'Y')) { redo(); return true; }
            if (ev.key == kKeyEscape) { if (helpOpen) helpOpen = false; else if (popup.open) popup.open = false; else if (kpan.open) kpan.open = false; else if (menuOpen) menuOpen = false; else if (learnMode) setLearn(false); repaint(); return true; }
            return false;
        }
        if (ev.key == kKeyEnter) {
            if (saving && savingKind == 1) {
                float fv[kXlfoFields]; for (int f = 0; f < compFieldCount(savingCompKind); ++f) fv[f] = val[compParam(savingCompKind, savingCompIdx, f)];
                if (saveCompPreset(savingCompKind, saveName, fv)) { saving = false; savingKind = 0; saveError = false; flash("Saved " + sanitizeName(saveName)); } else saveError = true;
            } else if (saving && savingKind == 2) {
                float fv[kTapPatternFields]; readTapPattern(val, fv);
                if (saveTapPattern(saveName, fv)) { saving = false; savingKind = 0; saveError = false; flash("Saved tap pattern " + sanitizeName(saveName)); } else saveError = true;
            } else if (saving) {
                if (saveUserPreset(saveName, val)) { userPresets = listUserPresets(); curUser = sanitizeName(saveName); curFactory = -1; std::copy(val, val + kParamCount, userVals); saving = false; saveError = false; }
                else saveError = true;
            } else endEdit(true);
            repaint(); return true;
        }
        if (ev.key == kKeyEscape) { if (saving) { saving = false; savingKind = 0; } else endEdit(false); repaint(); return true; }
        if (ev.key == kKeyBackspace || ev.key == kKeyDelete) { if (!buf->empty()) buf->pop_back(); repaint(); return true; }
        if (ev.key >= 32 && ev.key < 127 && buf->size() < (saving ? 40u : 12u)) {
            char ch = (char)ev.key;
            if ((ev.mod & kModifierShift) && std::isalpha((unsigned char)ch)) ch = (char)std::toupper((unsigned char)ch);
            *buf += ch; repaint(); return true;
        }
        return true;
    }

    bool onMouse(const MouseEvent& ev) override {
        const float s = uiScale(), x = ev.pos.getX() / s, y = ev.pos.getY() / s;
        if (saving) return true;
        if (ev.press) { flashUntil = 0; tipShown = false; tipSuppressed = tipKey; }
        if (helpOpen) { if (ev.press && ev.button == 1) helpClick(x, y); return true; }
        if (ev.button == 2 && ev.press) {                  // right-click
            if (popup.open && listPopup()) { const int m = popupHit(x, y); compPendingDel = (m >= 0 && compItems[m].type == 1) ? m : -1; repaint(); return true; }
            if (menuOpen) { const int m = menuHit(x, y); if (m >= 0 && menuItem(m).kind == 1) { pendingDelete = menuItem(m).idx; repaint(); } return true; }
            if (learnMode) { const int i = hit(x, y); uint32_t p = kParamCount;
                if (i >= 0) p = effParam(controls()[i]); else if (inRect(x, y, FRZ_X, FRZ_Y, FRZ_W, FRZ_H)) p = kFreeze; else { const int ft = filterTabHit(x, y); if (ft >= 0) p = fpar(ft, kFOn); }
                if (p < kParamCount) { char b[24]; snprintf(b, sizeof b, "unmap:%u", p); setState("cmd", b); paramCC[p] = -1; repaint(); }
                return true; }
            const int th = tapHit(x, y);
            if (th == 100) { beginGesture(); commit(kMainOn, val[kMainOn] > 0.5f ? 0.f : 1.f); endGesture(); flash(val[kMainOn] > 0.5f ? "Main tap on" : "Main tap off"); return true; }
            if (th >= 0) { beginGesture(); commit(tpar(th, kTOn), 0); endGesture(); char b[32]; snprintf(b, sizeof b, "Tap %d off", th + 1); flash(b); if (selTap == th) selTap = -1; return true; }
            { const int i = hit(x, y); if (i >= 0 && controls()[i].type == Knob) { popup.open = false; openKnobPanel(effParam(controls()[i]), x, y); return true; } }
            return false;
        }
        if (ev.button != 1) return false;
        if (!ev.press) {
            if (dragSrc > 0) { const int i = hit(x, y);
                if (i >= 0) assign(dragSrc, effParam(controls()[i]));
                else { const int sp_ = slotPartHit(x, y);
                    if (sp_ >= 0 && sp_ % 10 == 3) { const int id = sp_ / 10;
                        if (dragSrc >= kClassicChip && isClassicRow(id)) assignClassic(kSrcLfo1 + dragSrc - kClassicChip, kDstSlotFirst + id - 100);
                        else if (dragSrc < kClassicChip && !isClassicRow(id)) assignDst(dragSrc, kV2SlotFirst + id);
                        else flash("Classic sources modulate classic slots; the others modulate the new slots"); } }
                dragSrc = -1; repaint(); return true; }
            if (active >= 0) { editParameter(effParam(controls()[active]), false); active = -1; endGesture(); return true; }
            if (activeRow >= 0) { editParameter(rowParam(activeRow, kSlotAmt), false); activeRow = -1; endGesture(); return true; }
            if (dragTap >= 0) { if (dragTap == 100) editParameter(kMainLevel, false); else { editParameter(tpar(dragTap, kTTime), false); editParameter(tpar(dragTap, kTLevel), false); } dragTap = -1; endGesture(); return true; }
            if (dragStep) { dragStep = false; endGesture(); return true; }
            if (dragXy) { dragXy = false; endGesture(); return true; }
            return false;
        }
        if (editing >= 0) endEdit(true);
        if (popup.open) { const int i = popupHit(x, y); if (i >= 0) popupPick(i); else if (!popupInside(x, y)) popup.open = false; repaint(); return true; }
        if (kpan.open) {
            const int part = kpHit(x, y);
            if (part < 0) { if (!kpInside(x, y)) kpan.open = false; repaint(); return true; }
            rowClick(part, x, y, ev.mod);
            if (rowsFor(kpan.param).empty()) kpan.open = false;
            repaint(); return true;
        }
        if (menuOpen) {
            const int m = menuHit(x, y);
            if (m >= 0) { const MRect r = menuItem(m);
                if (r.kind == 3) return true;                       // the search field: just type
                if ((r.kind == 0 || r.kind == 1 || r.kind == 4) && x >= r.x + r.w - 20) { toggleFav(favKey(r.kind, r.idx)); return true; }
                if (r.kind == 2) { menuOpen = false; saving = true; savingKind = 0; saveError = false; saveName = curUser; }
                else if (r.kind == 1 && pendingDelete == r.idx) { deleteUserPreset(userPresets[r.idx]); if (curUser == userPresets[r.idx]) curUser.clear(); userPresets = listUserPresets(); pendingDelete = -1; repaint(); return true; }
                else { menuOpen = false; if (r.kind == 0) applyFactory(r.idx); else if (r.kind == 1) applyUser(userPresets[r.idx]); else applyFavourite(r.idx); } }
            else menuOpen = false;
            pendingDelete = -1; menuHover = -1; repaint(); return true;
        }
        const int hh = headerHit(x, y);
        if (hh == 1) { setLearn(!learnMode); return true; }
        if (hh == 2) { beginGesture(); commit(kMidiOn, val[kMidiOn] > 0.5f ? 0.f : 1.f); endGesture(); return true; }
        if (hh == 3) { undo(); return true; }
        if (hh == 4) { redo(); return true; }
        if (hh == 5) { helpOpen = true; repaint(); return true; }
        if (hh == 6 || hh == 7) { switchAB(hh - 6); return true; }
        if (hh == 8) { copyAB(); return true; }
        const int pb = presetBarHit(x, y);
        if (pb == 0) { step(-1); return true; }
        if (pb == 2) { step(1); return true; }
        if (pb == 1) { openMenu(); return true; }
        if (clearHit(x, y)) { setState("cmd", "clear"); for (int& m : paramCC) m = -1; flash("All MIDI assignments cleared"); return true; }
        if (inRect(x, y, FRZ_X, FRZ_Y, FRZ_W, FRZ_H)) { if (learnMode) { learn(kFreeze); return true; } beginGesture(); commit(kFreeze, val[kFreeze] > 0.5f ? 0.f : 1.f); endGesture(); return true; }
        { bool dot = false; const int ft = filterTabHit(x, y, &dot);
          if (ft >= 0) { if (learnMode) { learn(fpar(ft, kFOn)); return true; }
              if (dot) { beginGesture(); commit(fpar(ft, kFOn), fOn(ft) ? 0.f : 1.f); endGesture(); }
              selF = ft; repaint(); return true; } }
        { const int v = viewHit(x, y); if (v >= 0) { tapView = v == 1; repaint(); return true; } }
        const int tb = tabHit(x, y); if (tb >= 0) { tab = tb; repaint(); return true; }
        if (!learnMode) {
            if (inRect(x, y, TAP_X, TAP_Y, TAP_W, TAP_H)) { tapTempo(ev.time); return true; }
            if (inRect(x, y, CLR2_X, CLR2_Y, CLR2_W, CLR2_H)) { setState("cmd", "panic"); flash("Echoes cleared"); return true; }
            if (patHit(x, y)) { openListPopup(6, PAT_X, PAT_Y, PAT_W); return true; }
            if (compHit(x, y)) { openListPopup(3, COMP_X, COMP_Y, COMP_W); return true; }
            const int ch = chipHit(x, y); if (ch > 0) { dragSrc = ch; dragPX = x; dragPY = y; repaint(); return true; }
            if (styleHit(x, y)) { popup.open = true; popup.kind = 0; popup.slot = -1; popup.x = STYLE_X; popup.y = STYLE_Y + 30; popup.hover = -1; repaint(); return true; }
            const int part = slotPartHit(x, y); if (part >= 0) { rowClick(part, x, y, ev.mod); return true; }
        }
        int seg = 0; const int i = hit(x, y, &seg);
        if (i >= 0 && !learnMode && ringHit(controls()[i], x, y)) { openKnobPanel(effParam(controls()[i]), x, y); return true; }
        if (i >= 0) {
            const Ctl& c = controls()[i]; const uint32_t p = effParam(c);
            if (learnMode) { learn(p); return true; }
            if (c.type == Toggle) { beginGesture(); commit(p, val[p] > 0.5f ? 0.f : 1.f); endGesture(); return true; }
            if (c.type == Segment) {
                beginGesture();
                if (c.group == GFilt && c.field == kFSlope) commit(p, seg == 0 ? (float)kSlope6 : (float)(seg - 1));
                else commit(p, (float)seg);
                endGesture(); return true; }
            if (c.type == Stepper) { const ParamDef& d = paramDef(p); float v = val[p] + seg; if (v > d.max) v = d.min; if (v < d.min) v = d.max; beginGesture(); commit(p, v); endGesture(); return true; }
            if (ev.mod & (kModifierControl | kModifierSuper)) { beginGesture(); commit(p, defv[p]); endGesture(); return true; }
            if (i == lastClick && ev.time - lastClickTime < 350) {
                lastClick = -1; editing = i;
                char b[48]; fmt(p, b, sizeof b); editBuf = b;
                const size_t spc = editBuf.find(' '); if (spc != std::string::npos && !isPan(p)) editBuf.resize(spc);
                if (!editBuf.empty() && editBuf.back() == '%') editBuf.pop_back();
                if (std::strcmp(paramDef(p).unit, "Hz") == 0 && val[p] >= 1000) { char kb[16]; snprintf(kb, sizeof kb, "%.2fk", val[p] / 1000); editBuf = kb; }
                if (isPan(p)) { char pb2[16]; snprintf(pb2, sizeof pb2, "%s%.0f", val[p] < 0 ? "L" : "R", std::fabs(val[p]) * 100); editBuf = std::fabs(val[p]) < .005f ? "C" : pb2; }
                if (p == kTimeL || p == kTimeR) { char tb2[16]; snprintf(tb2, sizeof tb2, "%.1f", val[p]); editBuf = tb2; }
                if (isTimeParam(p) && val[p] < 1) { char tb2[16]; snprintf(tb2, sizeof tb2, "%.0fms", val[p] * 1000); editBuf = tb2; }
                repaint(); return true;
            }
            lastClick = i; lastClickTime = ev.time;
            beginGesture(); active = i; dragY = y; dragNorm = toNorm(p, val[p]);
            const float dist = std::sqrt((x - c.x) * (x - c.x) + (y - c.y) * (y - c.y));
            circular = c.type == Knob && c.r > 19 && dist > c.r * 0.62f && !(paramDef(p).hints & kHintInteger);
            editParameter(p, true);
            if (circular) setValue(p, fromNorm(p, angleNorm(c, x, y)));
            repaint(); return true;
        }
        if (learnMode) return false;
        if (stepHit(x, y)) { beginGesture(); dragStep = true; setStepAt(x, y, (ev.mod & (kModifierControl | kModifierSuper)) != 0); return true; }
        if (xyHit(x, y)) { beginGesture(); dragXy = true; setXyAt(x, y); return true; }
        if (tapView && inRect(x, y, DISP_X, DISP_Y, DISP_W, DISP_H)) {
            const int th = tapHit(x, y);
            if (th >= 0) {
                beginGesture(); dragTap = th; dragX = x; dragY = y; if (th < 100) selTap = th;
                if (th == 100) editParameter(kMainLevel, true); else { editParameter(tpar(th, kTTime), true); editParameter(tpar(th, kTLevel), true); }
                repaint(); return true;
            }
            if (ev.time - lastDispClick < 350 && std::fabs(x - lastDispX) < 6) { lastDispClick = 0; addTapAt(x, y); return true; }
            lastDispClick = ev.time; lastDispX = x; selTap = -1; repaint(); return true;
        }
        selSlot = -1;
        return false;
    }

    bool onMotion(const MotionEvent& ev) override {
        const float s = uiScale(), x = ev.pos.getX() / s, y = ev.pos.getY() / s;
        if (saving) return true;
        if (helpOpen) { const int h = helpHit(x, y); if (h != helpHover) { helpHover = h; repaint(); } return true; }
        if (dragSrc > 0) { dragPX = x; dragPY = y; hover = hit(x, y); repaint(); return true; }
        if (popup.open) { const int h = popupHit(x, y); if (h != popup.hover) { popup.hover = h; repaint(); } return true; }
        if (kpan.open && activeRow < 0) { const int h = kpHit(x, y); if (h != hoverKp) { hoverKp = h; repaint(); } return true; }
        const float fine = (ev.mod & kModifierShift) ? 0.15f : 1.f;
        if (active >= 0) {
            const Ctl& c = controls()[active]; const uint32_t p = effParam(c);
            if (circular) { setValue(p, fromNorm(p, angleNorm(c, x, y))); return true; }
            float sens = 220.f;
            if (paramDef(p).hints & kHintInteger) sens = std::max(60.f, 14.f * (paramDef(p).max - paramDef(p).min));
            dragNorm = clampf(dragNorm + (dragY - y) / sens * fine, 0.f, 1.f); dragY = y; setValue(p, fromNorm(p, dragNorm));
            return true;
        }
        if (activeRow >= 0) { const uint32_t ap = rowParam(activeRow, kSlotAmt); dragNorm = clampf(dragNorm + (dragY - y) / 440.f * fine, 0.f, 1.f); dragY = y; setValue(ap, fromNorm(ap, dragNorm)); return true; }
        if (dragTap >= 0) {
            const float nx = dragX + (x - dragX) * fine, ny = dragY + (y - dragY) * fine; dragX = nx; dragY = ny;
            if (dragTap == 100) setValue(kMainLevel, std::round(dbOfY(ny) * 10.f) / 10.f);
            else { setValue(tpar(dragTap, kTTime), tOfX(nx)); setValue(tpar(dragTap, kTLevel), std::round(dbOfY(ny) * 10.f) / 10.f); }
            return true;
        }
        if (dragStep) { setStepAt(x, y, false); return true; }
        if (dragXy) { setXyAt(x, y); return true; }
        if (menuOpen) { const int m = menuHit(x, y); if (m != menuHover) { menuHover = m; repaint(); } return true; }
        const int hh = headerHit(x, y); if (hh != hoverHeader) { hoverHeader = hh; repaint(); }
        const int pb = presetBarHit(x, y); if (pb != presetHover) { presetHover = pb; repaint(); }
        const bool hc = clearHit(x, y); if (hc != hoverClear) { hoverClear = hc; repaint(); }
        const int h = hit(x, y); if (h != hover) { hover = h; repaint(); }
        const int ft = filterTabHit(x, y); if (ft != hoverFTab) { hoverFTab = ft; repaint(); }
        const int tb = tabHit(x, y); if (tb != hoverTab) { hoverTab = tb; repaint(); }
        const int ch = chipHit(x, y); if (ch != hoverChip) { hoverChip = ch; repaint(); }
        const bool hcp = compHit(x, y); if (hcp != hoverComp) { hoverComp = hcp; repaint(); }
        const bool hpt = patHit(x, y); if (hpt != hoverPat) { hoverPat = hpt; repaint(); }
        const bool hs = styleHit(x, y); if (hs != hoverStyle) { hoverStyle = hs; repaint(); }
        const int vh = viewHit(x, y); if (vh != hoverView) { hoverView = vh; repaint(); }
        const int th = tapHit(x, y); if (th != hoverTapIdx) { hoverTapIdx = th; repaint(); }
        const bool hf = inRect(x, y, FRZ_X, FRZ_Y, FRZ_W, FRZ_H); if (hf != hoverFrz) { hoverFrz = hf; repaint(); }
        const bool ht = inRect(x, y, TAP_X, TAP_Y, TAP_W, TAP_H); if (ht != hoverTap) { hoverTap = ht; repaint(); }
        const bool h2 = inRect(x, y, CLR2_X, CLR2_Y, CLR2_W, CLR2_H); if (h2 != hoverClr2) { hoverClr2 = h2; repaint(); }
        const int part = slotPartHit(x, y); if (part != hoverSlotPart) { hoverSlotPart = part; repaint(); }
        // tips
        int key = -1;
        if (h >= 0 && ringHit(controls()[h], x, y)) key = 4000 + kTipModRing;
        else if (h >= 0) key = h;
        else if (hh > 0) key = 1000 + hh;
        else if (pb >= 0) key = 2000 + pb;
        else if (ch > 0) key = 3000 + ch;
        else if (hs) key = 5000;
        else if (hf) key = 5001;
        else if (part >= 0) key = 10000 + part;
        else { const int a = areaHit(x, y); if (a >= 0) key = 4000 + a; }
        tipX = x; tipY = y;
        if (key != tipKey) { const bool was = tipShown; tipKey = key; tipSince = nowSec(); tipShown = false; if (key != tipSuppressed) tipSuppressed = -2; if (was) repaint(); }
        return false;
    }

    bool onScroll(const ScrollEvent& ev) override {
        const float s = uiScale(), sx = ev.pos.getX() / s, sy = ev.pos.getY() / s;
        if (menuOpen || saving || helpOpen || popup.open) return true;
        const float dir = ev.delta.getY() > 0 ? 1.f : -1.f;
        if (presetBarHit(sx, sy) >= 0) { step(dir > 0 ? -1 : 1); return true; }
        if (inRect(sx, sy, SX, SY, SW, SH)) { slotScroll = (int)clampf((float)(slotScroll - (int)dir), 0.f, (float)maxScroll()); repaint(); return true; }
        { const int th = tapHit(sx, sy);
          if (th >= 0) { const uint32_t p = th == 100 ? kMainPan : tpar(th, kTPan); beginGesture(); commit(p, clampf(std::round((val[p] + dir * 0.05f) * 20.f) / 20.f, -1.f, 1.f)); endGesture();
              char b[24]; fmt(p, b, sizeof b); flash(std::string(th == 100 ? "Main tap pan " : "Tap pan ") + b); return true; } }
        const int i = hit(sx, sy); if (i < 0) return false;
        const Ctl& c = controls()[i]; const uint32_t p = effParam(c);
        if (c.type == Stepper) { const ParamDef& d = paramDef(p); float v = val[p] + dir; if (v > d.max) v = d.min; if (v < d.min) v = d.max; beginGesture(); commit(p, v); endGesture(); return true; }
        if (c.type != Knob) return false;
        float st = (ev.mod & kModifierShift) ? 0.004f : 0.025f;
        if (paramDef(p).hints & kHintInteger) st = 1.f / (paramDef(p).max - paramDef(p).min);
        beginGesture(); commit(p, fromNorm(p, toNorm(p, val[p]) + dir * st)); endGesture();
        return true;
    }

    void uiIdle() override {
        const double t = nowSec();
        if (t - lastPoll > 0.033) {
            lastPoll = t;
            if (StrataPlugin* const dsp = (StrataPlugin*)getPluginInstancePointer()) {
                const Engine& e = dsp->engine;
                std::copy(e.off2, e.off2 + kNumDests2, liveOff); std::copy(e.srcVal2, e.srcVal2 + em::kNumSources, liveSrc);
                std::copy(e.srcVal, e.srcVal + kNumSources, liveClassic);
                for (int f = 0; f < kNumFilters; ++f) liveHz[f] = e.liveHz[f];
                for (int l = 0; l < em::kNumXlfo; ++l) livePhase[l] = e.xlfoPhase(l);
                liveFrozen = e.frozen();
            }
            repaint();
        }
        if (flashing() != flashShown) { flashShown = flashing(); repaint(); }
        if (gTipsOn && !tipShown && tipKey >= 0 && tipKey != tipSuppressed && active < 0 && dragSrc <= 0 && dragTap < 0 && !menuOpen && !saving && !helpOpen && !popup.open && !kpan.open && editing < 0
            && t - tipSince > 0.6) { tipShown = true; repaint(); }
    }

    // ---- tips --------------------------------------------------------------------
    bool tipContent(std::string& title, std::string& value, std::string& body) const {
        if (tipKey < 0) return false;
        char b[64];
        if (tipKey < 1000) { const Ctl& c = controls()[tipKey]; const uint32_t p = effParam(c); fmt(p, b, sizeof b); title = paramDef(p).name; value = b; body = paramTip(p);
            if (c.type == Knob) body += "\n\nDrag up/down (Shift for fine) or grab the rim of a large knob. Double-click to type, Cmd/Ctrl-click to reset.";
            if (c.type == Knob && (destForParam2(p) > 0 || destForParam(p) > 0)) body += " Drop a source chip here to modulate it; right-click to list what modulates it.";
            if (dimmed(p)) body += "\n\n(Not active with the current settings.)";
            if (paramCC[p] >= 0) { char cc[24]; snprintf(cc, sizeof cc, "   ·   CC%d", paramCC[p]); value += cc; }
            return true; }
        if (tipKey >= 10000) { const int id = (tipKey - 10000) / 10, k = (tipKey - 10000) % 10;
            if (isClassicRow(id)) { title = "Classic slot " + std::to_string(id - 99); value.clear(); body = std::string(targetTip(kTipClassicRow)) + (k == 5 ? "\n\nThe cross removes it." : ""); return true; }
            static const int f[6] = { 0, kSlotSrc, kSlotDst, kSlotAmt, kSlotBypass, kSlotSrc };
            title = "Slot " + std::to_string(id + 1); value.clear(); body = k == 5 ? "Removes this modulation." : paramTip(sp(id, f[k])); return true; }
        if (tipKey == 5000) { const int st = (int)std::round(val[fpar(selF, kFStyle)]); title = "Style"; value = styleName(st); body = std::string(styleTip(st)) + "\n\nClick to choose another style."; return true; }
        if (tipKey == 5001) { title = "Freeze"; value = val[kFreeze] > 0.5f ? "On" : "Off"; body = paramTip(kFreeze); if (paramCC[kFreeze] >= 0) { snprintf(b, sizeof b, "   ·   CC%d", paramCC[kFreeze]); value += b; } return true; }
        if (tipKey >= 3000 && tipKey < 4000) { const int src = tipKey - 3000; title = chipName(src); value.clear(); body = targetTip(src >= kClassicChip ? kTipClassicChip : kTipChip); return true; }
        int t;
        if (tipKey >= 4000) t = tipKey - 4000;
        else if (tipKey >= 2000) t = kTipPrev + (tipKey - 2000);
        else { const int hh = tipKey - 1000; t = hh == 1 ? kTipLearn : hh == 2 ? kTipMidi : hh == 3 ? kTipUndo : hh == 4 ? kTipRedo : hh == 5 ? kTipHelp : hh == 6 ? kTipA : hh == 7 ? kTipB : kTipCopy; }
        title = targetTitle(t); value.clear(); body = targetTip(t);
        if (t == kTipUndo || t == kTipRedo) { const size_t n = t == kTipUndo ? undoStack.size() : redoStack.size(); snprintf(b, sizeof b, "%d step%s", (int)n, n == 1 ? "" : "s"); value = b; }
        return true;
    }
    void drawTooltip() {
        std::string title, value, body;
        if (!tipContent(title, value, body)) return;
        const float w = 330, pad = 12;
        fontSize(11); textLetterSpacing(0.2f); textLineHeight(1.3f);
        float bb[4]; textBoxBounds(0, 0, w - 2 * pad, body.c_str(), nullptr, bb);
        const float h = pad + 16 + 8 + (bb[3] - bb[1]) + pad;
        float x = tipX + 16, y = tipY + 20;
        if (x + w > W - 8) x = tipX - w - 12;
        if (y + h > H - 8) y = tipY - h - 12;
        x = clampf(x, 8, W - w - 8); y = clampf(y, 8, H - h - 8);
        beginPath(); rect(x - 10, y - 4, w + 20, h + 18); fillPaint(boxGradient(x, y + 3, w, h, 6, 14, Color(0, 0, 0, 0.55f), Color(0, 0, 0, 0.f))); fill();
        beginPath(); roundedRect(x, y, w, h, 6); fillColor(Color(0x22, 0x20, 0x1d)); fill(); strokeColor(C_LINE); strokeWidth(1); stroke();
        fontSize(10); textLetterSpacing(1.6f); textAlign(ALIGN_LEFT | ALIGN_TOP); fillColor(C_TEXT);
        std::string up = title; for (char& ch : up) ch = (char)std::toupper((unsigned char)ch);
        text(x + pad, y + pad + 1, up.c_str(), nullptr);
        if (!value.empty()) { fontSize(10.5f); textLetterSpacing(0.4f); textAlign(ALIGN_RIGHT | ALIGN_TOP); fillColor(C_ACC); text(x + w - pad, y + pad + 1, value.c_str(), nullptr); }
        fontSize(11); textLetterSpacing(0.2f); textLineHeight(1.3f); textAlign(ALIGN_LEFT | ALIGN_TOP); fillColor(C_DIM);
        textBox(x + pad, y + pad + 24, w - 2 * pad, body.c_str(), nullptr);
        textLineHeight(1.f);
    }

    // ---- help window ---------------------------------------------------------------
    static constexpr float HX = 240, HY = 110, HW = 760, HH = 580, HTAB_W = 180, HTAB_H = 34;
    int helpHit(float x, float y) const {
        if (x < HX || x > HX + HW || y < HY || y > HY + HH) return -2;
        for (int i = 0; i < kNumHelpPages; ++i) if (x >= HX + 12 && x < HX + 12 + HTAB_W && y >= HY + 56 + i * HTAB_H && y < HY + 56 + (i + 1) * HTAB_H) return i;
        const float by = HY + HH - 44;
        if (y >= by && y < by + 28) {
            if (x >= HX + 12 && x < HX + 12 + HTAB_W) return 100;
            if (x >= HX + HW - 330 && x < HX + HW - 140) return 101;
            if (x >= HX + HW - 124 && x < HX + HW - 16) return 102;
        }
        if (x >= HX + HW - 40 && y < HY + 40) return 102;
        return -1;
    }
    void helpClick(float x, float y) {
        const int h = helpHit(x, y);
        if (h >= 0 && h < kNumHelpPages) helpPage = h;
        else if (h == 100) gTipsOn = !gTipsOn;
        else if (h == 101) openURL(kManualURL);
        else if (h == 102 || h == -2) helpOpen = false;
        repaint();
    }
    void drawHelp() {
        beginPath(); rect(0, 0, W, H); fillColor(Color(0, 0, 0, 0.55f)); fill();
        beginPath(); roundedRect(HX, HY, HW, HH, 10); fillColor(C_PANEL); fill(); strokeColor(C_LINE); strokeWidth(1); stroke();
        fontSize(13); textLetterSpacing(4.f); textAlign(ALIGN_LEFT | ALIGN_MIDDLE); fillColor(C_TEXT); text(HX + 22, HY + 28, "STRATA HELP", nullptr);
        const float cx = HX + HW - 24, cy = HY + 26;
        beginPath(); moveTo(cx - 6, cy - 6); lineTo(cx + 6, cy + 6); moveTo(cx + 6, cy - 6); lineTo(cx - 6, cy + 6); strokeColor(helpHover == 102 ? C_TEXT : C_DIM); strokeWidth(1.6f); stroke();
        for (int i = 0; i < kNumHelpPages; ++i) {
            const float ty = HY + 56 + i * HTAB_H;
            if (i == helpPage || i == helpHover) { beginPath(); roundedRect(HX + 12, ty + 2, HTAB_W, HTAB_H - 4, 5); fillColor(i == helpPage ? withAlpha(C_ACC, 0.16f) : C_PANEL_HI); fill(); }
            if (i == helpPage) { beginPath(); roundedRect(HX + 12, ty + 8, 3, HTAB_H - 16, 1.5f); fillColor(C_ACC); fill(); }
            fontSize(11.5f); textLetterSpacing(0.6f); textAlign(ALIGN_LEFT | ALIGN_MIDDLE); fillColor(i == helpPage ? C_TEXT : C_DIM);
            text(HX + 28, ty + HTAB_H / 2 + 0.5f, kHelpPages[i].title, nullptr);
        }
        beginPath(); moveTo(HX + HTAB_W + 26, HY + 56); lineTo(HX + HTAB_W + 26, HY + HH - 56); strokeColor(C_LINE); strokeWidth(1); stroke();
        const float bx = HX + HTAB_W + 46, bw = HW - HTAB_W - 70;
        fontSize(15); textLetterSpacing(0.8f); textAlign(ALIGN_LEFT | ALIGN_TOP); fillColor(C_ACC); text(bx, HY + 60, kHelpPages[helpPage].title, nullptr);
        fontSize(12); textLetterSpacing(0.2f); textLineHeight(1.38f); fillColor(Color(0xcc, 0xc8, 0xc0));
        textBox(bx, HY + 92, bw, kHelpPages[helpPage].body, nullptr);
        textLineHeight(1.f);
        const float by = HY + HH - 44;
        beginPath(); moveTo(HX + 12, by - 10); lineTo(HX + HW - 12, by - 10); strokeColor(C_LINE); stroke();
        const float tw = 34, th = 18, tx = HX + 16, tyy = by + 14 - th / 2;
        beginPath(); roundedRect(tx, tyy, tw, th, th / 2); fillColor(gTipsOn ? withAlpha(C_ACC, 0.25f) : C_PANEL_HI); fill(); strokeColor(gTipsOn ? C_ACC : C_LINE); stroke();
        beginPath(); circle(gTipsOn ? tx + tw - th / 2 : tx + th / 2, by + 14, 6); fillColor(gTipsOn ? C_ACC : C_DIM); fill();
        fontSize(10); textLetterSpacing(1.6f); textAlign(ALIGN_LEFT | ALIGN_MIDDLE); fillColor(helpHover == 100 ? C_TEXT : C_DIM); text(tx + tw + 10, by + 14.5f, "HOVER TIPS", nullptr);
        auto btn = [&](float x0, float w0, const char* label, bool hot, bool primary) {
            beginPath(); roundedRect(x0, by, w0, 28, 5); fillColor(primary ? (hot ? withAlpha(C_ACC, 0.32f) : withAlpha(C_ACC, 0.16f)) : (hot ? C_PANEL_HI : C_PANEL)); fill();
            strokeColor(primary ? C_ACC : C_LINE); strokeWidth(1); stroke();
            fontSize(9.5f); textLetterSpacing(1.8f); textAlign(ALIGN_CENTER | ALIGN_MIDDLE); fillColor(C_TEXT); text(x0 + w0 / 2, by + 14.5f, label, nullptr);
        };
        btn(HX + HW - 330, 190, "FULL MANUAL (WEB)", helpHover == 101, false);
        btn(HX + HW - 124, 108, "CLOSE", helpHover == 102, true);
    }

private:
    float val[kParamCount], defv[kParamCount], userVals[kParamCount] = {}, before[kParamCount];
    int paramCC[kParamCount];
    int active = -1, hover = -1, lastClick = -1, editing = -1;
    uint lastClickTime = 0, lastDispClick = 0; float lastDispX = 0;
    float dragX = 0, dragY = 0, dragNorm = 0; bool circular = false, gesture = false;
    int selF = 0, hoverFTab = -1, tab = 0, hoverTab = -1;
    bool tapView = false; int hoverView = -1, hoverTapIdx = -1, dragTap = -1, selTap = -1; bool hoverPat = false, hoverFrz = false;
    int dragSrc = -1, hoverChip = -1; float dragPX = 0, dragPY = 0;
    int selSlot = -1, slotScroll = 0, hoverSlotPart = -1, activeRow = -1;
    bool hoverStyle = false, hoverComp = false, dragStep = false, dragXy = false;
    std::vector<Snap> undoStack, redoStack; Snap abStore[2]; int abSlot = 0;
    std::string editBuf, statusText; double flashUntil = 0; bool flashShown = false;
    int curFactory = 0, presetHover = -1, menuHover = -1, pendingDelete = -1, hoverHeader = 0;
    std::string curUser, query; std::vector<std::string> userPresets, favs;
    bool menuOpen = false, saving = false, saveError = false; std::string saveName;
    bool learnMode = false, hoverClear = false, hoverTap = false, hoverClr2 = false; int learnWaiting = -1; float lastLearnEcho = 0;
    int tipKey = -1, tipSuppressed = -2; double tipSince = 0; bool tipShown = false; float tipX = 0, tipY = 0;
    bool helpOpen = false; int helpPage = 0, helpHover = -1;
    double taps[5] = {}; int nTaps = 0, tapFlash = 0;
    float liveOff[kNumDests2], liveSrc[em::kNumSources], liveClassic[kNumSources], liveHz[kNumFilters]; double livePhase[em::kNumXlfo] = {}; bool liveFrozen = false;
    double lastPoll = 0;
    ResizeHandle resizeHandle;

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(StrataUI)
};

UI* createUI() { return new StrataUI(); }

END_NAMESPACE_DISTRHO
