// Presets for a single modulation source (an XLFO, an envelope generator or an envelope follower):
// a factory list per kind plus user files, each holding only that source's fields. License: MIT.
#pragma once
#include "UserPresets.hpp"

namespace strata {

enum CompKind { kCompXlfo, kCompEg, kCompEf, kNumCompKinds };

inline int compFieldCount(int kind) { return kind == kCompXlfo ? (int)kXlfoFields : (kind == kCompEg ? (int)kEgFields : (int)kEfFields); }
inline uint32_t compParam(int kind, int index, int field) {
    return kind == kCompXlfo ? lp_(index, field) : (kind == kCompEg ? ep(index, field) : fp(index, field));
}
// the symbol of a field without its instance prefix ("l1rate" -> "rate", "eg2a" -> "a")
inline std::string compFieldSym(int kind, int field) {
    const std::string s = paramDef(compParam(kind, 0, field)).sym;
    return s.substr(kind == kCompEf ? 3 : 2);     // "x1rate", "e1a", "ef1a"
}
inline em::FieldRange compRange(int kind, int field) { return kind == kCompXlfo ? em::xlfoRange(field) : (kind == kCompEg ? em::egRange(field) : em::efRange(field)); }
inline const char* compKindName(int kind) { static const char* const n[kNumCompKinds] = { "LFO", "Envelope", "Follower" }; return n[kind]; }

struct CompPreset { const char* name; int kind; std::vector<std::pair<int, float>> set; };

inline const std::vector<CompPreset>& compFactory() {
    static const std::vector<CompPreset> p = [] {
        std::vector<CompPreset> v;
        auto L = [&](const char* n, std::vector<std::pair<int, float>> s) { v.push_back({ n, kCompXlfo, s }); };
        auto E = [&](const char* n, std::vector<std::pair<int, float>> s) { v.push_back({ n, kCompEg, s }); };
        auto F = [&](const char* n, std::vector<std::pair<int, float>> s) { v.push_back({ n, kCompEf, s }); };
        // XLFO: waves (Wave 0 sine, 1 tri, 2 saw up, 3 saw down, 4 square, 5 random, 6 S&H; divisions 4 = 1 bar, 6 = 1/4, 7 = 1/8, 8 = 1/16)
        L("Slow Sine", { { em::kLWave, 0 }, { em::kLRate, 0.25f } });
        L("Quarter Pulse", { { em::kLWave, 4 }, { em::kLSync, 1 }, { em::kLDiv, 6 } });
        L("Eighth Saw Down", { { em::kLWave, 3 }, { em::kLSync, 1 }, { em::kLDiv, 7 } });
        L("Bar Triangle", { { em::kLWave, 1 }, { em::kLSync, 1 }, { em::kLDiv, 4 } });
        L("Dotted Eighth Sine", { { em::kLWave, 0 }, { em::kLSync, 2 }, { em::kLDiv, 7 } });
        L("Triplet Square", { { em::kLWave, 4 }, { em::kLSync, 3 }, { em::kLDiv, 7 } });
        L("Random Drift", { { em::kLWave, 5 }, { em::kLRate, 0.4f } });
        L("Sample & Hold 1/16", { { em::kLWave, 6 }, { em::kLSync, 1 }, { em::kLDiv, 8 } });
        {   // step sequences: one cycle plays every step
            std::vector<std::pair<int, float>> g = { { em::kLMode, 1 }, { em::kLSync, 1 }, { em::kLDiv, 4 }, { em::kLSteps, 16 }, { em::kLGlide, 0.05f } };
            static const float gate[16] = { 1, 1, -1, 1, 1, -1, 1, 1, 1, -1, 1, 1, -1, 1, -1, -1 };
            for (int s = 0; s < 16; ++s) g.push_back({ em::kLStep1 + s, gate[s] });
            L("Trance Gate", g);
            std::vector<std::pair<int, float>> r = { { em::kLMode, 1 }, { em::kLSync, 1 }, { em::kLDiv, 3 }, { em::kLSteps, 8 }, { em::kLGlide, 0 } };
            for (int s = 0; s < 8; ++s) r.push_back({ em::kLStep1 + s, -1 + 2 * s / 7.f });
            L("Rising Steps", r);
            std::vector<std::pair<int, float>> m = { { em::kLMode, 1 }, { em::kLSync, 1 }, { em::kLDiv, 4 }, { em::kLSteps, 8 }, { em::kLGlide, 0.8f } };
            static const float mel[8] = { 0.2f, -0.6f, 0.9f, -0.1f, 0.5f, -0.9f, 0.7f, -0.3f };
            for (int s = 0; s < 8; ++s) m.push_back({ em::kLStep1 + s, mel[s] });
            L("Smooth Steps", m);
        }
        // EG: trigger 0 = MIDI notes, 1 = audio transients
        E("Transient Pluck", { { em::kETrigger, 1 }, { em::kEThreshold, -30 }, { em::kEAttack, 0.002f }, { em::kEDecay, 0.15f }, { em::kESustain, 0 }, { em::kERelease, 0.1f } });
        E("Transient Swell", { { em::kETrigger, 1 }, { em::kEThreshold, -30 }, { em::kEAttack, 0.6f }, { em::kEDecay, 0.5f }, { em::kESustain, 0.8f }, { em::kERelease, 1.2f } });
        E("Long Fade", { { em::kETrigger, 1 }, { em::kEThreshold, -36 }, { em::kEAttack, 0.01f }, { em::kEDecay, 3 }, { em::kESustain, 0.2f }, { em::kERelease, 5 } });
        E("Note Pluck", { { em::kETrigger, 0 }, { em::kEAttack, 0.002f }, { em::kEDecay, 0.25f }, { em::kESustain, 0 }, { em::kERelease, 0.2f } });
        E("Note Swell", { { em::kETrigger, 0 }, { em::kEAttack, 1.5f }, { em::kEDecay, 0.5f }, { em::kESustain, 1 }, { em::kERelease, 1.5f } });
        E("Note Gate", { { em::kETrigger, 0 }, { em::kEAttack, 0.002f }, { em::kEDecay, 0.01f }, { em::kESustain, 1 }, { em::kERelease, 0.05f } });
        // EF: source 0 = input, 1 = sidechain
        F("Fast Follow", { { em::kFAttack, 0.001f }, { em::kFRelease, 0.05f } });
        F("Smooth Follow", { { em::kFAttack, 0.02f }, { em::kFRelease, 0.4f } });
        F("Slow Follow", { { em::kFAttack, 0.1f }, { em::kFRelease, 1.5f } });
        F("Hot Follow", { { em::kFAttack, 0.005f }, { em::kFRelease, 0.15f }, { em::kFGain, 12 } });
        F("Sidechain Fast", { { em::kFSource, 1 }, { em::kFAttack, 0.002f }, { em::kFRelease, 0.2f }, { em::kFGain, 6 } });
        F("Sidechain Smooth", { { em::kFSource, 1 }, { em::kFAttack, 0.03f }, { em::kFRelease, 0.5f } });
        return v;
    }();
    return p;
}
inline std::vector<int> compFactoryFor(int kind) {
    std::vector<int> r; const std::vector<CompPreset>& f = compFactory();
    for (int i = 0; i < (int)f.size(); ++i) if (f[i].kind == kind) r.push_back(i);
    return r;
}
// field values of a factory preset: defaults for everything it does not set
inline void compFactoryValues(int i, float* out) {
    const CompPreset& c = compFactory()[i];
    for (int f = 0; f < compFieldCount(c.kind); ++f) out[f] = compRange(c.kind, f).def;
    for (const auto& kv : c.set) out[kv.first] = kv.second;
}

static const char* const kCompExt = ".stratamod";
inline std::string compDir(int kind) { return userPresetDir() + "/" + compKindName(kind); }

inline std::vector<std::string> listCompPresets(int kind) {
    std::vector<std::string> out;
    DIR* d = opendir(compDir(kind).c_str());
    if (!d) return out;
    const size_t el = std::strlen(kCompExt);
    while (dirent* e = readdir(d)) {
        std::string n = e->d_name;
        if (n.size() > el && n.compare(n.size() - el, el, kCompExt) == 0) out.push_back(n.substr(0, n.size() - el));
    }
    closedir(d);
    std::sort(out.begin(), out.end());
    return out;
}
inline bool saveCompPreset(int kind, const std::string& rawName, const float* fields) {
    const std::string name = sanitizeName(rawName);
    if (name.empty()) return false;
    makeDirs(compDir(kind));
    FILE* f = std::fopen((compDir(kind) + "/" + name + kCompExt).c_str(), "wb");
    if (!f) return false;
    std::fprintf(f, "# Strata %s preset\nformat=1\n", compKindName(kind));
    for (int i = 0; i < compFieldCount(kind); ++i) std::fprintf(f, "%s=%.6g\n", compFieldSym(kind, i).c_str(), fields[i]);
    std::fclose(f);
    return true;
}
// missing fields take their defaults
inline bool loadCompPreset(int kind, const std::string& name, float* fields) {
    FILE* f = std::fopen((compDir(kind) + "/" + name + kCompExt).c_str(), "rb");
    if (!f) return false;
    for (int i = 0; i < compFieldCount(kind); ++i) fields[i] = compRange(kind, i).def;
    char line[256];
    while (std::fgets(line, sizeof line, f)) {
        char* eq = std::strchr(line, '=');
        if (!eq || line[0] == '#') continue;
        *eq = 0; char* nl = std::strpbrk(eq + 1, "\r\n"); if (nl) *nl = 0;
        for (int i = 0; i < compFieldCount(kind); ++i)
            if (compFieldSym(kind, i) == line) { const em::FieldRange r = compRange(kind, i); fields[i] = clampf((float)std::atof(eq + 1), r.min, r.max); break; }
    }
    std::fclose(f);
    return true;
}
inline bool deleteCompPreset(int kind, const std::string& name) { return std::remove((compDir(kind) + "/" + name + kCompExt).c_str()) == 0; }

} // namespace strata
