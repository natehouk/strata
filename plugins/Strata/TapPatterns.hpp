// Tap patterns: the main tap plus the 15 extra taps, saved and recalled as one unit.
// A factory list plus user files in the "Taps" folder. License: MIT.
#pragma once
#include "UserPresets.hpp"
#include <cmath>

namespace strata {

static constexpr int kTapPatternFields = 3 + kNumTaps * kTapFields;     // main on/level/pan, then each tap's fields
inline uint32_t tapPatternParam(int i) { return i < 3 ? (uint32_t)(kMainOn + i) : tpar((i - 3) / kTapFields, (i - 3) % kTapFields); }

struct TapPattern { const char* name; float v[kTapPatternFields]; };

inline const std::vector<TapPattern>& tapFactory() {
    static const std::vector<TapPattern> p = [] {
        std::vector<TapPattern> v;
        // start: main tap on at 0 dB, every extra tap off at its engine default
        auto base = [](const char* n) { TapPattern t; t.name = n; t.v[0] = 1; t.v[1] = 0; t.v[2] = 0;
            for (int k = 0; k < kNumTaps; ++k) { float* f = t.v + 3 + k * kTapFields; f[kTOn] = 0; f[kTTime] = (k + 1) / 16.f; f[kTLevel] = -6; f[kTPan] = 0; }
            return t; };
        auto tap = [](TapPattern& t, int k, float time, float db, float pan) { float* f = t.v + 3 + k * kTapFields; f[kTOn] = 1; f[kTTime] = time; f[kTLevel] = db; f[kTPan] = pan; };
        { TapPattern t = base("Main Only"); v.push_back(t); }
        { TapPattern t = base("Even Quarters"); for (int k = 0; k < 3; ++k) { tap(t, k, (k + 1) / 4.f, -6, k % 2 ? 0.5f : -0.5f); } v.push_back(t); }
        { TapPattern t = base("Even Eighths"); for (int k = 0; k < 7; ++k) { tap(t, k, (k + 1) / 8.f, -9, (k % 2 ? 1 : -1) * 0.6f); } v.push_back(t); }
        { TapPattern t = base("Sixteenths Ramp Up"); for (int k = 0; k < 15; ++k) { tap(t, k, (k + 1) / 16.f, -30 + 1.8f * k, 0); } v.push_back(t); }
        { TapPattern t = base("Sixteenths Fade Out"); for (int k = 0; k < 15; ++k) { tap(t, k, (k + 1) / 16.f, -1.5f * k, (k % 2 ? 1 : -1) * 0.3f); } t.v[1] = -24; v.push_back(t); }
        { TapPattern t = base("Tresillo"); tap(t, 0, 3 / 8.f, -4, -0.6f); tap(t, 1, 6 / 8.f, -4, 0.6f); v.push_back(t); }
        { TapPattern t = base("Swing Eighths"); tap(t, 0, 2 / 6.f * 1.0f, -6, -0.4f); tap(t, 1, 0.5f, -4, 0.4f); tap(t, 2, 0.5f + 2 / 6.f, -8, -0.4f); v.push_back(t); }
        { TapPattern t = base("Pre-Echo Cluster"); for (int k = 0; k < 4; ++k) { tap(t, k, 0.8f + 0.04f * k, -18 + 4 * k, k % 2 ? 0.3f : -0.3f); } v.push_back(t); }
        { TapPattern t = base("Stereo Scatter"); static const float tm[8] = { .11f, .19f, .29f, .37f, .52f, .61f, .74f, .89f };
          for (int k = 0; k < 8; ++k) { tap(t, k, tm[k], -8 - (k % 3) * 2, (k % 2 ? 1 : -1) * (0.4f + 0.075f * k)); } v.push_back(t); }
        { TapPattern t = base("Golden Spread"); for (int k = 0; k < 10; ++k) { const float g = std::fmod((k + 1) * 0.618034f, 1.f); tap(t, k, std::max(g, 0.03f), -10, std::sin(k * 2.4f) * 0.8f); } v.push_back(t); }
        { TapPattern t = base("Cloud"); for (int k = 0; k < 15; ++k) { const float g = std::fmod((k + 1) * 0.381966f + 0.05f, 1.f); tap(t, k, std::max(g, 0.02f), -14 + std::sin(k * 1.7f) * 4, std::sin(k * 3.1f)); } t.v[1] = -6; v.push_back(t); }
        { TapPattern t = base("Swell Into Main"); for (int k = 0; k < 7; ++k) { tap(t, k, 0.3f + 0.1f * k, -36 + 5 * k, 0); } v.push_back(t); }
        { TapPattern t = base("Taps Only Triplets"); tap(t, 0, 1 / 3.f, 0, -0.7f); tap(t, 1, 2 / 3.f, -3, 0.7f); t.v[0] = 0; v.push_back(t); }
        return v;
    }();
    return p;
}

inline void readTapPattern(const float* p, float* out) { for (int i = 0; i < kTapPatternFields; ++i) out[i] = p[tapPatternParam(i)]; }

inline std::string tapDir() { return userPresetDir() + "/Taps"; }
static const char* const kTapExt = ".stratataps";

inline std::vector<std::string> listTapPatterns() {
    std::vector<std::string> out;
    DIR* d = opendir(tapDir().c_str());
    if (!d) return out;
    const size_t el = std::strlen(kTapExt);
    while (dirent* e = readdir(d)) {
        std::string n = e->d_name;
        if (n.size() > el && n.compare(n.size() - el, el, kTapExt) == 0) out.push_back(n.substr(0, n.size() - el));
    }
    closedir(d);
    std::sort(out.begin(), out.end());
    return out;
}
inline bool saveTapPattern(const std::string& rawName, const float* fields) {
    const std::string name = sanitizeName(rawName);
    if (name.empty()) return false;
    makeDirs(tapDir());
    FILE* f = std::fopen((tapDir() + "/" + name + kTapExt).c_str(), "wb");
    if (!f) return false;
    std::fprintf(f, "# Strata tap pattern\nformat=1\n");
    for (int i = 0; i < kTapPatternFields; ++i) std::fprintf(f, "%s=%.6g\n", paramDef(tapPatternParam(i)).sym, fields[i]);
    std::fclose(f);
    return true;
}
// missing fields take the "Main Only" values
inline bool loadTapPattern(const std::string& name, float* fields) {
    FILE* f = std::fopen((tapDir() + "/" + name + kTapExt).c_str(), "rb");
    if (!f) return false;
    std::copy(tapFactory()[0].v, tapFactory()[0].v + kTapPatternFields, fields);
    char line[256];
    while (std::fgets(line, sizeof line, f)) {
        char* eq = std::strchr(line, '=');
        if (!eq || line[0] == '#') continue;
        *eq = 0; char* nl = std::strpbrk(eq + 1, "\r\n"); if (nl) *nl = 0;
        for (int i = 0; i < kTapPatternFields; ++i) {
            const ParamDef& d = paramDef(tapPatternParam(i));
            if (std::strcmp(d.sym, line) == 0) { fields[i] = clampf((float)std::atof(eq + 1), d.min, d.max); break; }
        }
    }
    std::fclose(f);
    return true;
}
inline bool deleteTapPattern(const std::string& name) { return std::remove((tapDir() + "/" + name + kTapExt).c_str()) == 0; }

} // namespace strata
