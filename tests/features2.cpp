// 3.0 features: taps, six filters with shapes, slopes and per-channel routing, freeze, the loop effects
// (lo-fi, diffuse, dynamics, pitch), mid/side and width, ping-pong right, wet level and pan, and the v2
// modulation system (XLFO on time, sidechain follower ducking the wet signal).
#include "../plugins/Strata/ParamDefs.hpp"
#include <cstdio>
#include <memory>
#include <vector>
using namespace strata;
static int fails = 0;
#define CHECK(c, ...) do { bool ok_ = (c); printf("%s  ", ok_ ? "PASS" : "FAIL"); printf(__VA_ARGS__); printf("\n"); if (!ok_) fails++; } while (0)
static const int SR = 48000;
typedef std::unique_ptr<Engine> EP;
struct Buf { std::vector<float> l, r; };
static Buf run(Engine& e, const std::vector<float>& xl, const std::vector<float>& xr, const float* sc = nullptr) {
    Buf b; b.l.resize(xl.size()); b.r.resize(xl.size());
    for (size_t i = 0; i < xl.size(); i += 256) { const uint32_t n = (uint32_t)std::min<size_t>(256, xl.size() - i);
        e.process(xl.data() + i, xr.data() + i, sc ? sc + i : nullptr, sc ? sc + i : nullptr, b.l.data() + i, b.r.data() + i, n); }
    return b;
}
static Buf run(Engine& e, const std::vector<float>& x) { return run(e, x, x); }
static double rms(const std::vector<float>& v, size_t a, size_t z) { double s = 0; for (size_t i = a; i < z; ++i) s += v[i] * (double)v[i]; return std::sqrt(s / (z - a)); }
static std::vector<float> sine(int n, float f, float amp) { std::vector<float> v(n); for (int i = 0; i < n; ++i) v[i] = amp * std::sin(2 * kPi * f * i / SR); return v; }
static std::vector<float> noise(int n, float a, uint32_t seed = 1) { std::vector<float> v(n); uint32_t r = seed; for (auto& x : v) { r = r * 1664525u + 1013904223u; x = a * ((int32_t)r * (1.f / 2147483648.f)); } return v; }
static std::vector<float> impulse(int n, float amp = 0.5f) { std::vector<float> x(n, 0.f); x[100] = amp; return x; }
static float db(double a, double b) { return (float)(20 * std::log10(a / b + 1e-30)); }
// a plain delay: 400 ms, no filters, no drive, no feedback, full wet, hard-panned lines
static EP plain(float ms = 400) {
    EP e(new Engine()); e->setSampleRate(SR);
    e->p[kF1On] = 0; e->p[kF2On] = 0; e->p[kDrive] = 0; e->p[kMix] = 1; e->p[kPanL] = -1; e->p[kPanR] = 1;
    e->p[kSyncL] = 0; e->p[kSyncR] = 0; e->p[kTimeL] = ms; e->p[kTimeR] = ms; e->p[kFbL] = 0; e->p[kFbR] = 0; e->p[kGlide] = 0.001f;
    e->reset(); return e;
}
static float bandDb(const std::vector<float>& v, int a, int z, float f) { double c = 0, s = 0; for (int i = a; i < z; ++i) { const double w = 2 * M_PI * f * i / SR; c += v[i] * std::cos(w); s += v[i] * std::sin(w); } return (float)(10 * std::log10((c * c + s * s) / ((z - a) * (double)(z - a)) + 1e-30)); }

int main() {
    // ---- taps
    { EP e = plain(); e->p[tpar(0, kTOn)] = 1; e->p[tpar(0, kTTime)] = 0.25f; e->p[tpar(0, kTLevel)] = 0;
      e->p[tpar(1, kTOn)] = 1; e->p[tpar(1, kTTime)] = 0.5f; e->p[tpar(1, kTLevel)] = -6; e->reset();
      Buf b = run(*e, impulse(SR)); const int t1 = 100 + 100 * 48, t2 = 100 + 200 * 48, t3 = 100 + 400 * 48;
      auto pk = [&](int c) { float m = 0; for (int i = c - 3; i <= c + 3; ++i) m = std::max(m, std::fabs(b.l[i])); return m; };
      CHECK(pk(t1) > 0.45f && std::fabs(db(pk(t2), pk(t1)) + 6) < 0.6f && pk(t3) > 0.45f, "taps at 100 and 200 ms (%.2f, %.1f dB) and the main tap at 400 ms (%.2f)", pk(t1), db(pk(t2), pk(t1)), pk(t3)); }
    { EP e = plain(); e->p[tpar(0, kTOn)] = 1; e->p[tpar(0, kTTime)] = 0.5f; e->p[tpar(0, kTPan)] = -1; e->p[kMainOn] = 0; e->reset();
      Buf b = run(*e, impulse(SR)); const int t1 = 100 + 200 * 48, t3 = 100 + 400 * 48;
      float pl = 0, pr = 0; for (int i = t1 - 3; i <= t1 + 3; ++i) { pl = std::max(pl, std::fabs(b.l[i])); pr = std::max(pr, std::fabs(b.r[i])); }
      CHECK(pr < 1e-4f && pl > 0.2f && rms(b.l, t3 - 100, t3 + 100) < 1e-5, "tap panned left (L %.2f, R %.5f); main tap off removes the 400 ms echo", pl, pr); }
    // ---- filters
    { auto bright = [](int nf) { EP e = plain(); for (int f = 0; f < nf; ++f) { e->p[fpar(f, kFOn)] = 1; e->p[fpar(f, kFType)] = kLowPass; e->p[fpar(f, kFFreq)] = 1000; e->p[fpar(f, kFStyle)] = 6; e->p[fpar(f, kFPeak)] = 0; } e->reset();
          Buf b = run(*e, sine(SR, 4000, 0.3f)); return rms(b.l, SR / 2, SR); };
      const double one = bright(1), six = bright(6);
      CHECK(db(six, one) < -50 && one > 1e-4, "six filters in series: %.0f dB more attenuation at 4 kHz than one", db(six, one)); }
    { EP e = plain(); e->p[kF1On] = 1; e->p[kF1Type] = kBell; e->p[kF1Freq] = 1000; e->p[fpar(0, kFGain)] = 12; e->p[kF1Peak] = 0.3f; e->reset();
      Buf b = run(*e, sine(SR, 1000, 0.1f)); EP o = plain(); Buf c = run(*o, sine(SR, 1000, 0.1f));
      EP e2 = plain(); e2->p[kF1On] = 1; e2->p[kF1Type] = kBell; e2->p[kF1Freq] = 1000; e2->p[fpar(0, kFGain)] = 12; e2->p[kF1Peak] = 0.3f; e2->reset(); Buf b2 = run(*e2, sine(SR, 100, 0.1f)); EP o2 = plain(); Buf c2 = run(*o2, sine(SR, 100, 0.1f));
      CHECK(std::fabs(db(rms(b.l, SR / 2, SR), rms(c.l, SR / 2, SR)) - 12) < 1 && std::fabs(db(rms(b2.l, SR / 2, SR), rms(c2.l, SR / 2, SR))) < 1, "bell +12 dB in the loop: %.1f dB at 1 kHz, %.1f dB at 100 Hz",
            db(rms(b.l, SR / 2, SR), rms(c.l, SR / 2, SR)), db(rms(b2.l, SR / 2, SR), rms(c2.l, SR / 2, SR))); }
    { EP e = plain(); e->p[kF1On] = 1; e->p[kF1Type] = kNotch; e->p[kF1Freq] = 1000; e->p[kF1Style] = 6; e->reset(); Buf b = run(*e, sine(SR, 1000, 0.3f));
      EP s = plain(); s->p[kF1On] = 1; s->p[kF1Type] = kLowPass; s->p[kF1Slope] = kSlope6; s->p[kF1Freq] = 500; s->p[kF1Style] = 6; s->reset();
      Buf b1 = run(*s, sine(SR, 4000, 0.3f)); EP s2 = plain(); s2->p[kF1On] = 1; s2->p[kF1Type] = kLowPass; s2->p[kF1Slope] = kSlope6; s2->p[kF1Freq] = 500; s2->p[kF1Style] = 6; s2->reset(); Buf b2 = run(*s2, sine(SR, 8000, 0.3f));
      const float oct = db(rms(b1.l, SR / 2, SR), rms(b2.l, SR / 2, SR));
      CHECK(rms(b.l, SR / 2, SR) < 0.003 && std::fabs(oct - 6) < 0.8f, "notch removes 1 kHz (%.4f); 6 dB/oct low pass: %.1f dB/oct", rms(b.l, SR / 2, SR), oct); }
    { EP e = plain(); e->p[kRouting] = kPerChannel; e->p[kF1On] = 1; e->p[kF1Type] = kLowPass; e->p[kF1Freq] = 200; e->p[kF1Style] = 6; e->p[kF1Peak] = 0; e->reset();
      Buf b = run(*e, sine(SR, 4000, 0.3f));
      CHECK(rms(b.l, SR / 2, SR) < 0.01 && rms(b.r, SR / 2, SR) > 0.15, "per-channel routing: filter 1 on the left only (L %.4f, R %.3f)", rms(b.l, SR / 2, SR), rms(b.r, SR / 2, SR)); }
    // ---- freeze
    { EP e = plain(300); e->p[kFbL] = 0.4f; e->p[kFbR] = 0.4f; e->reset();
      std::vector<float> x = noise(SR * 3, 0.3f); for (int i = SR / 2; i < SR * 3; ++i) x[i] = (i < SR) ? 0.f : 0.6f * std::sin(0.3f * i);
      std::vector<float> L(x.size()), R(x.size());
      for (int i = 0; i < (int)x.size(); i += 256) { if (i >= SR / 2 + 300 * 48 - 256 && i < SR / 2 + 300 * 48) e->p[kFreeze] = 1; e->process(x.data() + i, x.data() + i, L.data() + i, R.data() + i, std::min(256, (int)x.size() - i)); }
      const double a = rms(L, SR, SR + SR / 2), b2 = rms(L, 2 * SR + SR / 2, 3 * SR);
      const float tone = bandDb(L, 2 * SR, 3 * SR, 0.3f * SR / (2 * kPi));
      CHECK(std::fabs(db(b2, a)) < 1.0f && a > 0.05 && tone < -50, "freeze holds the loop (%.2f dB over 1.5 s) and keeps new input out (%.0f dB)", db(b2, a), tone); }
    // ---- loop effects
    { auto harm = [](bool on) { EP e = plain(); e->p[kLofiOn] = on; e->p[kLofi] = 0.8f; e->reset(); Buf b = run(*e, sine(SR, 997, 0.4f));
          const double all = rms(b.l, SR / 2, SR); const double f0 = std::pow(10.0, bandDb(b.l, SR / 2, SR, 997) / 10.0) * 2.0;   // fundamental power (A^2/2)
          return (float)(10 * std::log10(std::max(all * all - f0, 1e-20) / f0)); };
      const float off = harm(false), on = harm(true);
      CHECK(on > -30 && off < -60, "lo-fi adds distortion products (%.0f -> %.0f dB below the fundamental)", off, on); }
    { auto crest = [](bool on) { EP e = plain(); e->p[kDiffOn] = on; e->p[kDiffuse] = 1; e->reset(); Buf b = run(*e, impulse(SR)); int a = 100 + 400 * 48 - 50, z = a + 4000; float pk = 0; for (int i = a; i < z; ++i) pk = std::max(pk, std::fabs(b.l[i])); return db(pk, rms(b.l, a, z)); };
      const float c0 = crest(false), c1 = crest(true);
      CHECK(c1 < c0 - 10, "diffuse smears the echo (crest factor %.0f -> %.0f dB)", c0, c1); }
    { auto echoDiff = [](float dyn) { auto lvl = [&](float amp) { EP e = plain(); e->p[kDynOn] = dyn != 0; e->p[kDyn] = dyn; e->reset(); Buf b = run(*e, sine(SR, 500, amp)); return rms(b.l, SR / 2, SR); };
          return db(lvl(0.5f), lvl(0.05f)); };
      const float plainD = echoDiff(0), swell = echoDiff(1);
      // ducking: a 0.5 s burst; its echoes (0.4 - 0.9 s) are held down while the burst plays and come back after it
      auto duckRatio = [](float dyn) { EP e = plain(); e->p[kDynOn] = dyn != 0; e->p[kDyn] = dyn; e->reset();
          std::vector<float> in = sine(SR, 500, 0.5f); for (int i = SR / 2; i < SR; ++i) in[i] = 0;
          Buf b = run(*e, in); return db(rms(b.l, (int)(0.7f * SR), (int)(0.85f * SR)), rms(b.l, (int)(0.42f * SR), (int)(0.49f * SR))); };
      const float d0 = duckRatio(0), d1 = duckRatio(-1);
      CHECK(std::fabs(plainD - 20) < 0.5f && swell < 8 && std::fabs(d0) < 1 && d1 > 15,
            "dynamics: swell turns 20 dB of input range into %.1f dB; duck holds the echoes %.0f dB down while the input plays", swell, d1); }
    { EP e = plain(); e->p[kPitchOn] = 1; e->p[kPitch] = 12; e->reset(); Buf b = run(*e, sine(SR, 440, 0.3f));
      EP m = plain(); m->p[kPitchOn] = 1; m->p[kPitch] = 12; m->p[kPitchInLoop] = 0; m->p[kPitchMirror] = 1; m->reset(); Buf c = run(*m, sine(SR, 440, 0.3f));
      // a grain shifter spreads the shifted tone over a few Hz either side, so compare energy in bands
      auto band = [](const std::vector<float>& v, float f) { double e = 0; for (float k = f * 0.9f; k <= f * 1.1f; k += 2.5f) e += std::pow(10.0, bandDb(v, SR / 2, SR, k) / 10.0); return (float)(10 * std::log10(e + 1e-30)); };
      const float up = band(b.l, 880) - band(b.l, 440), down = band(c.r, 220) - band(c.r, 440);
      CHECK(up > 15 && down > 15, "pitch +12: echo an octave up (%.0f dB above 440 Hz); mirrored outside the loop: right an octave down (%.0f dB)", up, down); }
    // ---- channels
    { EP e = plain(); e->p[kChanMode] = 1; e->p[kRouting] = kPerChannel; e->p[kF1On] = 1; e->p[kF1Type] = kLowPass; e->p[kF1Freq] = 200; e->p[kF1Style] = 6; e->p[kF1Peak] = 0; e->reset();
      std::vector<float> x = sine(SR, 4000, 0.3f), nx(x.size()); for (size_t i = 0; i < x.size(); ++i) nx[i] = -x[i];
      Buf side = run(*e, x, nx); EP e2 = plain(); e2->p[kChanMode] = 1; e2->p[kRouting] = kPerChannel; e2->p[kF1On] = 1; e2->p[kF1Type] = kLowPass; e2->p[kF1Freq] = 200; e2->p[kF1Style] = 6; e2->p[kF1Peak] = 0; e2->reset();
      Buf mid = run(*e2, x, x);
      CHECK(rms(side.l, SR / 2, SR) > 0.1 && rms(mid.l, SR / 2, SR) < 0.01, "mid/side: filter 1 on the mid only (side echo %.3f, mid echo %.4f)", rms(side.l, SR / 2, SR), rms(mid.l, SR / 2, SR)); }
    { EP e = plain(); e->p[kWidth] = 0; e->p[kPanL] = 0; e->p[kPanR] = 0; e->reset(); std::vector<float> x = noise(SR, 0.3f, 3), y = noise(SR, 0.3f, 7); Buf b = run(*e, x, y);
      double dd = 0; for (int i = SR / 2; i < SR; ++i) dd += (b.l[i] - b.r[i]) * (b.l[i] - b.r[i]);
      CHECK(dd < 1e-8, "width 0: the echoes are mono"); }
    { EP e = plain(); e->p[kInput] = 3; e->p[kFbL] = 0; e->reset(); Buf b = run(*e, impulse(SR));
      const int t = 100 + 400 * 48; CHECK(std::fabs(b.r[t]) > 0.9f && std::fabs(b.l[t]) < 1e-4f, "ping-pong R feeds the right line only"); }
    { EP e = plain(); e->p[kWetLevel] = -12; e->p[kWetPan] = 0.5f; e->reset(); Buf b = run(*e, sine(SR, 500, 0.3f)); EP o = plain(); Buf c = run(*o, sine(SR, 500, 0.3f));
      CHECK(std::fabs(db(rms(b.r, SR / 2, SR), rms(c.r, SR / 2, SR)) + 12) < 0.3f && std::fabs(db(rms(b.l, SR / 2, SR), rms(c.l, SR / 2, SR)) + 18) < 0.3f, "wet level -12 dB, wet pan right (L %.1f, R %.1f dB)",
            db(rms(b.l, SR / 2, SR), rms(c.l, SR / 2, SR)), db(rms(b.r, SR / 2, SR), rms(c.r, SR / 2, SR))); }
    // ---- modulation v2
    { EP e = plain(); e->p[lp_(0, em::kLRate)] = 2; e->setSlot2(0, em::kSrcXlfo1, kDstTimeL, 0.1f); e->reset();
      float lo = 1e9, hi = 0; std::vector<float> x(256, 0.f), L(256), R(256);
      for (int k = 0; k < SR / 256; ++k) { e->process(x.data(), x.data(), L.data(), R.data(), 256); lo = std::min(lo, e->liveTimeMs(0)); hi = std::max(hi, e->liveTimeMs(0)); }
      CHECK(hi / lo > 1.3f && hi / lo < 2.2f && std::fabs(e->liveTimeMs(1) - 400) < 0.5f, "XLFO 1 on left time: %.0f - %.0f ms, right stays at %.0f", lo, hi, e->liveTimeMs(1)); }
    { EP e = plain(); e->p[fp(0, em::kFSource)] = 1; e->setSlot2(0, em::kSrcEf1, kV2WetLevel, -0.8f); e->reset();
      std::vector<float> x = sine(SR * 2, 500, 0.3f), sc(SR * 2, 0.f); for (int i = SR; i < 2 * SR; ++i) sc[i] = 0.8f * std::sin(0.02f * i);
      Buf b = run(*e, x, x, sc.data());
      const float duck = db(rms(b.l, SR + SR / 2, 2 * SR), rms(b.l, SR / 2, SR));
      CHECK(duck < -9, "sidechain follower on wet level ducks the echoes (%.1f dB)", duck); }
    printf("%s (%d failures)\n", fails ? "FAILED" : "ALL PASSED", fails); return fails ? 1 : 0;
}
