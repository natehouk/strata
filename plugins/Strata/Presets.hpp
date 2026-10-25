// Factory presets (each lists only what it changes from the engine defaults). License: MIT.
// Engine defaults: left 1/8 dotted, right 1/4, 40% feedback, smooth LP 6 kHz + HP 150 Hz in the loop, 35% mix.
#pragma once
#include "StrataEngine.hpp"
#include "TapPatterns.hpp"
#include <algorithm>

namespace strata {

struct PV { uint32_t id; float v; };
struct PresetDef { const char* name; const char* category; const PV* pv; int taps; };   // taps: 1 + a factory tap pattern applied first, 0 none
struct Preset { const char* name; const char* category; float v[kParamCount]; };

static const char* const kCategories[] = { "Echo", "Ping-Pong", "Tape & Dub", "Modulation", "Filtered", "FX & Scratch", "Multi-Tap", "Space & Pitch" };
static constexpr int kNumCategories = 8;

#define SL(i, s, d, a) { kSlotFirst + (i) * 4 + 0, (float)(s) }, { kSlotFirst + (i) * 4 + 1, (float)(d) }, { kSlotFirst + (i) * 4 + 2, (a) }, { kSlotFirst + (i) * 4 + 3, 0 }
#define FREE(l, r) {kSyncL,0},{kSyncR,0},{kTimeL,l},{kTimeR,r}
#define NOFILT {kF1On,0},{kF2On,0}
#define END { kParamCount, 0 }
#define S2(i, s, d, a) { sp(i, kSlotSrc), (float)(s) }, { sp(i, kSlotDst), (float)(d) }, { sp(i, kSlotAmt), (a) }
namespace pv {
static const PV init[] = { END };

// ---------------------------------------------------------------- Echo
static const PV quarter[]   = { {kSyncL,1},{kDivL,4},{kLink,1},{kFbL,.35f},{kFbR,.35f},{kPanL,-.3f},{kPanR,.3f}, END };
static const PV dotted[]    = { {kSyncL,2},{kDivL,3},{kLink,1},{kFbL,.45f},{kFbR,.45f},{kPanL,-.2f},{kPanR,.2f}, END };
static const PV slap[]      = { FREE(95,110),{kFbL,.08f},{kFbR,.08f},{kMix,.3f},{kF1Freq,4500},{kPanL,-.4f},{kPanR,.4f}, END };
static const PV hall[]      = { FREE(700,730),{kFbL,.62f},{kFbR,.62f},{kXfLR,.1f},{kXfRL,.1f},{kF1Freq,3000},{kF1Slope,1},{kMix,.3f}, END };
static const PV triplets[]  = { {kSyncL,3},{kDivL,4},{kSyncR,3},{kDivR,3},{kFbL,.4f},{kFbR,.4f},{kPanL,-.7f},{kPanR,.7f}, END };
static const PV doubler[]   = { FREE(18,27),{kFbL,0},{kFbR,0},{kMix,.45f},{kPanL,-1},{kPanR,1},NOFILT,{kDrive,0}, END };
static const PV clean[]     = { {kSyncL,1},{kDivL,3},{kLink,1},{kFbL,.3f},{kFbR,.3f},NOFILT,{kDrive,0},{kPanL,-.5f},{kPanR,.5f}, END };

// ---------------------------------------------------------------- Ping-Pong (the input enters the left line and bounces)
#define PP {kInput,2},{kFbL,0},{kFbR,0},{kPanL,-1},{kPanR,1}
static const PV pp8[]       = { PP,{kSyncL,1},{kDivL,3},{kLink,1},{kXfLR,.6f},{kXfRL,.6f}, END };
static const PV ppDotted[]  = { PP,{kSyncL,2},{kDivL,3},{kLink,1},{kXfLR,.6f},{kXfRL,.6f},{kF1Freq,4000}, END };
static const PV ppTriplet[] = { PP,{kSyncL,3},{kDivL,3},{kLink,1},{kXfLR,.55f},{kXfRL,.55f}, END };
static const PV ppUneven[]  = { PP,{kSyncL,1},{kDivL,3},{kSyncR,1},{kDivR,2},{kXfLR,.6f},{kXfRL,.6f}, END };
static const PV ppWide[]    = { PP,{kSyncL,1},{kDivL,4},{kLink,1},{kXfLR,.5f},{kXfRL,.5f},{kF1Freq,2500},{kPanL,-.8f},{kPanR,.8f},{kMix,.3f}, END };

// ---------------------------------------------------------------- Tape & Dub (LFOs on the time for wow and flutter)
static const PV dub[]       = { {kSyncL,2},{kDivL,4},{kLink,1},{kFbL,.75f},{kFbR,.75f},{kF1Style,1},{kF1Freq,1800},{kF1Peak,.3f},{kF2Freq,300},{kDrive,.45f},
    {kMix,.4f},{kL2Wave,3},{kL2Rate,.3f},SL(0,kSrcLfo2,kDstTimeL,.001f),SL(1,kSrcLfo2,kDstTimeR,.001f), END };
static const PV warmTape[]  = { FREE(320,330),{kDrive,.5f},{kF1Style,1},{kF1Freq,3500},{kFbL,.5f},{kFbR,.5f},{kL1Wave,1},{kL1Rate,.7f},
    SL(0,kSrcLfo1,kDstTimeL,.001f),SL(1,kSrcLfo1,kDstTimeR,.001f), END };
static const PV cassette[]  = { FREE(260,275),{kDrive,.6f},{kF1Style,1},{kF1Freq,2500},{kF2Freq,400},{kFbL,.45f},{kFbR,.45f},{kL1Wave,1},{kL1Rate,.5f},
    {kL2Wave,3},{kL2Rate,7},SL(0,kSrcLfo1,kDstTimeL,.003f),SL(1,kSrcLfo1,kDstTimeR,.003f),SL(2,kSrcLfo2,kDstTimeL,.00004f),SL(3,kSrcLfo2,kDstTimeR,.00004f), END };
static const PV rolling[]   = { {kSyncL,1},{kDivL,4},{kSyncR,2},{kDivR,4},{kFbL,.85f},{kFbR,.85f},{kDrive,.7f},{kF1Style,3},{kF1Freq,2200},{kF1Peak,.2f},{kF2Freq,250},{kMix,.35f}, END };
static const PV runaway[]   = { {kSyncL,2},{kDivL,3},{kLink,1},{kFbL,1.02f},{kFbR,1.02f},{kF1Style,3},{kF1Type,1},{kF1Freq,900},{kF1Peak,.5f},{kF2On,0},{kDrive,.5f},{kMix,.35f}, END };
static const PV lofiSlap[]  = { FREE(120,135),{kF1Type,1},{kF1Freq,1200},{kF1Peak,.3f},{kF2Freq,600},{kDrive,.7f},{kFbL,.2f},{kFbR,.2f},{kMix,.4f}, END };
static const PV dubSpace[]  = { {kSyncL,1},{kDivL,4},{kSyncR,2},{kDivR,3},{kFbL,.7f},{kFbR,.7f},{kXfLR,.15f},{kXfRL,.15f},{kF1Style,1},{kF1Freq,1500},{kF2Freq,500},{kF2Slope,1},
    {kDrive,.4f},{kL1Wave,1},{kL1Sync,1},{kL1Div,3},SL(0,kSrcLfo1,kDstF1Freq,.25f), END };

// ---------------------------------------------------------------- Modulation (short times: chorus, flanger, phaser, vibrato)
#define MODBASE NOFILT,{kDrive,0},{kPanL,-1},{kPanR,1}
static const PV chorus[]    = { MODBASE,FREE(12,17),{kFbL,0},{kFbR,0},{kMix,.5f},{kL1Wave,1},{kL1Rate,.6f},{kL2Wave,1},{kL2Rate,.45f},
    SL(0,kSrcLfo1,kDstTimeL,.03f),SL(1,kSrcLfo2,kDstTimeR,.03f), END };
static const PV wideChorus[]= { MODBASE,FREE(20,26),{kFbL,.15f},{kFbR,.15f},{kXfLR,.1f},{kXfRL,.1f},{kMix,.5f},{kL1Wave,1},{kL1Rate,.25f},{kL2Wave,3},{kL2Rate,.8f},
    SL(0,kSrcLfo1,kDstTimeL,.04f),SL(1,kSrcLfo1,kDstTimeR,-.04f),SL(2,kSrcLfo2,kDstTimeL,.01f),SL(3,kSrcLfo2,kDstTimeR,.01f), END };
static const PV flanger[]   = { MODBASE,FREE(2,2),{kLink,1},{kFbL,.7f},{kFbR,.7f},{kMix,.5f},{kL1Rate,.12f},
    SL(0,kSrcLfo1,kDstTimeL,.2f),SL(1,kSrcLfo1,kDstTimeR,.2f),{kPanL,-.3f},{kPanR,.3f}, END };
static const PV jet[]       = { MODBASE,FREE(3,3),{kLink,1},{kFbL,.9f},{kFbR,.9f},{kMix,.5f},{kL1Rate,.07f},{kL1Balance,.8f},
    SL(0,kSrcLfo1,kDstTimeL,.25f),SL(1,kSrcLfo1,kDstTimeR,.23f), END };
static const PV negFlanger[]= { MODBASE,FREE(2.5f,2.5f),{kLink,1},{kFbL,.75f},{kFbR,.75f},{kInvL,1},{kInvR,1},{kMix,.5f},{kL1Rate,.15f},
    SL(0,kSrcLfo1,kDstTimeL,.2f),SL(1,kSrcLfo1,kDstTimeR,.2f), END };
static const PV phaser[]    = { MODBASE,FREE(1,1.3f),{kFbL,.6f},{kFbR,.6f},{kInvL,1},{kInvR,1},{kMix,.5f},{kL1Wave,1},{kL1Rate,.3f},
    SL(0,kSrcLfo1,kDstTimeL,.25f),SL(1,kSrcLfo1,kDstTimeR,-.25f), END };
static const PV vibrato[]   = { MODBASE,FREE(6,6),{kFbL,0},{kFbR,0},{kMix,1},{kL1Wave,1},{kL1Rate,5.5f},
    SL(0,kSrcLfo1,kDstTimeL,.01f),SL(1,kSrcLfo1,kDstTimeR,.01f),SL(2,kSrcWheel,kDstSlotFirst+0,.04f),SL(3,kSrcWheel,kDstSlotFirst+1,.04f), END };
static const PV rotary[]    = { MODBASE,FREE(3,3),{kFbL,.1f},{kFbR,.1f},{kMix,.6f},{kL1Wave,1},{kL1Rate,6},{kF1On,1},{kF1Freq,5000},
    SL(0,kSrcLfo1,kDstTimeL,.03f),SL(1,kSrcLfo1,kDstTimeR,-.03f),SL(2,kSrcLfo1,kDstPanL,.6f),SL(3,kSrcLfo1,kDstPanR,-.6f), END };

// ---------------------------------------------------------------- Filtered
static const PV sweepEcho[] = { {kF1Style,3},{kF1Type,1},{kF1Freq,900},{kF1Peak,.5f},{kFbL,.55f},{kFbR,.55f},{kL1Sync,1},{kL1Div,3},{kL1Wave,1},
    SL(0,kSrcLfo1,kDstF1Freq,.4f), END };
static const PV darkening[] = { {kSyncL,1},{kDivL,4},{kSyncR,2},{kDivR,4},{kFbL,.7f},{kFbR,.7f},{kF1Freq,1200},{kF1Slope,1}, END };
static const PV telephone[] = { {kF1Type,1},{kF1Freq,1500},{kF1Peak,.3f},{kF1Slope,1},{kF2Freq,500},{kF2Slope,1},{kFbL,.45f},{kFbR,.45f},{kMix,.45f}, END };
static const PV resoTaps[]  = { {kF1Style,3},{kF1Freq,700},{kF1Peak,.55f},{kFbL,.5f},{kFbR,.5f},{kL2Wave,4},{kL2Sync,1},{kL2Div,7},
    SL(0,kSrcLfo2,kDstF1Freq,.35f),{kMix,.4f}, END };
static const PV acidEcho[]  = { {kF1Style,4},{kF1Freq,500},{kF1Peak,.65f},{kF1Slope,1},{kFbL,.45f},{kFbR,.45f},{kEgThreshold,-30},{kEgDecay,.25f},{kEgSustain,.2f},
    SL(0,kSrcEg,kDstF1Freq,.5f), END };
static const PV envOpens[]  = { {kF1Freq,400},{kF1Slope,1},{kF1Peak,.35f},{kFbL,.55f},{kFbR,.55f},{kEgThreshold,-35},{kEgAttack,.05f},{kEgSustain,1},{kEgRelease,.8f},
    SL(0,kSrcEg,kDstF1Freq,.6f), END };
static const PV ducked[]    = { {kSyncL,1},{kDivL,4},{kLink,1},{kFbL,.5f},{kFbR,.5f},{kMix,.5f},{kEgThreshold,-40},{kEgAttack,.01f},{kEgSustain,1},{kEgRelease,.35f},
    SL(0,kSrcEg,kDstMix,-.45f), END };

// ---------------------------------------------------------------- FX & Scratch
static const PV scratch[]   = { {kGlide,.08f},{kFbL,.4f},{kFbR,.4f},{kL1Wave,4},{kL1Sync,1},{kL1Div,7},SL(0,kSrcLfo1,kDstTimeL,.25f),SL(1,kSrcLfo1,kDstTimeR,.25f),{kMix,.5f}, END };
static const PV tapeDive[]  = { {kGlide,.02f},{kFbL,.5f},{kFbR,.5f},{kL1Sync,1},{kL1Div,4},{kL1Balance,.98f},SL(0,kSrcLfo1,kDstTimeL,.25f),SL(1,kSrcLfo1,kDstTimeR,.25f),{kMix,.45f}, END };
static const PV stutter[]   = { {kMode,1},{kGlide,.01f},{kFbL,.5f},{kFbR,.5f},{kL1Wave,4},{kL1Sync,1},{kL1Div,8},SL(0,kSrcLfo1,kDstTimeL,.2f),SL(1,kSrcLfo1,kDstTimeR,.2f),{kMix,.5f}, END };
static const PV haze[]      = { {kMode,1},{kGlide,.03f},FREE(180,240),{kFbL,.7f},{kFbR,.7f},{kXfLR,.2f},{kXfRL,.2f},{kL2Wave,3},{kL2Rate,4},
    SL(0,kSrcLfo2,kDstTimeL,.15f),SL(1,kSrcLfo2,kDstTimeR,-.15f),{kMix,.5f},{kF1Freq,4000}, END };
static const PV loop[]      = { {kSyncL,1},{kDivL,6},{kLink,1},{kFbL,1},{kFbR,1},NOFILT,{kDrive,0},{kMix,.5f}, END };
static const PV wheelFb[]   = { {kFbL,.2f},{kFbR,.2f},SL(0,kSrcWheel,kDstFbL,.8f),SL(1,kSrcWheel,kDstFbR,.8f),{kF1Style,3},{kF1Freq,2500}, END };
static const PV keyComb[]   = { NOFILT,{kDrive,0},FREE(3.822f,3.822f),{kLink,1},{kFbL,.85f},{kFbR,.85f},{kMix,.45f},{kPanL,-.5f},{kPanR,.5f},
    SL(0,kSrcKey,kDstTimeL,-1),SL(1,kSrcKey,kDstTimeR,-1), END };
static const PV bendTime[]  = { {kGlide,.05f},SL(0,kSrcBend,kDstTimeL,.2f),SL(1,kSrcBend,kDstTimeR,.2f),SL(2,kSrcAftertouch,kDstFbL,.5f),SL(3,kSrcAftertouch,kDstFbR,.5f),{kMix,.45f}, END };
// ---------------------------------------------------------------- 3.0: Multi-Tap (tap pattern from TapPatterns.hpp, then these)
static const PV tapTres[]   = { {kSyncL,1},{kDivL,6},{kLink,1},{kFbL,.3f},{kFbR,.3f},{kMix,.35f}, END };
static const PV tapGolden[] = { {kSyncL,1},{kDivL,6},{kLink,1},{kFbL,.45f},{kFbR,.45f},{kMix,.35f},{kF1Freq,4500}, END };
static const PV tapCloud[]  = { FREE(900,960),{kFbL,.55f},{kFbR,.55f},{kDiffOn,1},{kDiffuse,.5f},{kMix,.4f},{kF1Freq,3500}, END };
static const PV tapSwell[]  = { {kSyncL,1},{kDivL,5},{kLink,1},{kFbL,.4f},{kFbR,.4f},{kMix,.35f}, END };
static const PV tapRamp[]   = { {kSyncL,1},{kDivL,6},{kLink,1},{kFbL,.2f},{kFbR,.2f},{kMix,.4f},{kF1Style,6},{kF1Freq,8000}, END };
// ---------------------------------------------------------------- 3.0: Space & Pitch
static const PV shimmer[]   = { FREE(420,460),{kFbL,.6f},{kFbR,.6f},{kPitchOn,1},{kPitch,12},{kPitchInLoop,1},{kDiffOn,1},{kDiffuse,.7f},{kF1Freq,7000},{kMix,.4f},{kWidth,1}, END };
static const PV octDown[]   = { {kSyncL,2},{kDivL,4},{kLink,1},{kFbL,.55f},{kFbR,.55f},{kPitchOn,1},{kPitch,-12},{kPitchInLoop,1},{kF1Style,1},{kF1Freq,2500},{kDrive,.3f},{kMix,.35f}, END };
static const PV fifths[]    = { {kSyncL,1},{kDivL,4},{kLink,1},{kFbL,.5f},{kFbR,.5f},{kPitchOn,1},{kPitch,7},{kPitchInLoop,1},{kPitchMirror,1},{kMix,.35f}, END };
static const PV wash[]      = { FREE(600,640),{kFbL,.75f},{kFbR,.75f},{kDiffOn,1},{kDiffuse,.9f},{kF1Freq,5000},{kF2Freq,250},{kMix,.4f}, END };
static const PV memory[]    = { {kSyncL,1},{kDivL,4},{kSyncR,2},{kDivR,4},{kFbL,.55f},{kFbR,.55f},{kLofiOn,1},{kLofi,.6f},{kF1Style,8},{kF1Freq,3000},{kMix,.4f}, END };
static const PV duck3[]     = { {kSyncL,2},{kDivL,3},{kLink,1},{kFbL,.45f},{kFbR,.45f},{kDynOn,1},{kDyn,-.7f},{kMix,.45f}, END };
static const PV swell3[]    = { {kSyncL,1},{kDivL,4},{kLink,1},{kFbL,.5f},{kFbR,.5f},{kDynOn,1},{kDyn,.35f},{kMix,.35f},{kOutGain,-3}, END };
static const PV freezePad[] = { FREE(250,333),{kFbL,.5f},{kFbR,.5f},{kDiffOn,1},{kDiffuse,.6f},{kMix,.5f},S2(0,em::kSrcWheel,kV2Freeze,1), END };
static const PV msWide[]    = { {kChanMode,1},{kSyncL,1},{kDivL,3},{kSyncR,2},{kDivR,3},{kFbL,.35f},{kFbR,.5f},{kPanL,0},{kPanR,0},{kWidth,1},{kMix,.35f}, END };
static const PV sixBand[]   = { {kSyncL,2},{kDivL,3},{kLink,1},{kFbL,.6f},{kFbR,.6f},{kF1Style,6},{kF1Freq,7000},{kF2Style,6},{kF2Freq,180},
    { fpar(2, kFOn), 1 }, { fpar(2, kFGain), 6 }, { fpar(2, kFFreq), 900 }, { fpar(3, kFOn), 1 }, { fpar(3, kFGain), -8 }, { fpar(3, kFFreq), 2800 },
    { fpar(4, kFOn), 1 }, { fpar(4, kFType), kLowShelf }, { fpar(4, kFGain), -6 }, { fpar(4, kFFreq), 300 }, { fpar(5, kFOn), 1 }, { fpar(5, kFType), kNotch }, { fpar(5, kFFreq), 5000 },{kMix,.35f}, END };
static const PV stepPan[]   = { {kSyncL,1},{kDivL,3},{kLink,1},{kFbL,.45f},{kFbR,.45f},{kMix,.4f},
    { lp_(0, em::kLMode), 1 }, { lp_(0, em::kLSync), 1 }, { lp_(0, em::kLDiv), 4 }, { lp_(0, em::kLSteps), 8 },
    { lp_(0, em::kLStep1), -1 }, { lp_(0, em::kLStep1 + 1), 1 }, { lp_(0, em::kLStep1 + 2), -.5f }, { lp_(0, em::kLStep1 + 3), .5f },
    { lp_(0, em::kLStep1 + 4), 0 }, { lp_(0, em::kLStep1 + 5), 1 }, { lp_(0, em::kLStep1 + 6), -1 }, { lp_(0, em::kLStep1 + 7), 0 },
    S2(0,em::kSrcXlfo1,kV2WetPan,.9f), END };
static const PV scDuck[]    = { {kSyncL,1},{kDivL,4},{kLink,1},{kFbL,.5f},{kFbR,.5f},{kMix,.5f},{ fp(0, em::kFSource), 1 },{ fp(0, em::kFGain), 12 },{ fp(0, em::kFRelease), .3f },
    S2(0,em::kSrcEf1,kV2WetLevel,-.75f), END };
} // namespace pv
#undef SL
#undef S2
#undef FREE
#undef NOFILT
#undef PP
#undef MODBASE
#undef END

static const PresetDef kPresetDefs[] = {
    {"Init", "Echo", pv::init, 0}, {"Quarter Echo", "Echo", pv::quarter, 0}, {"Dotted Eighth", "Echo", pv::dotted, 0}, {"Slapback", "Echo", pv::slap, 0},
    {"Long Hall", "Echo", pv::hall, 0}, {"Triplet Taps", "Echo", pv::triplets, 0}, {"Wide Doubler", "Echo", pv::doubler, 0}, {"Clean Digital", "Echo", pv::clean, 0},
    {"Ping-Pong 1/8", "Ping-Pong", pv::pp8, 0}, {"Ping-Pong Dotted", "Ping-Pong", pv::ppDotted, 0}, {"Triplet Bounce", "Ping-Pong", pv::ppTriplet, 0},
    {"Uneven Pong", "Ping-Pong", pv::ppUneven, 0}, {"Wide Bounce", "Ping-Pong", pv::ppWide, 0},
    {"Dub Echo", "Tape & Dub", pv::dub, 0}, {"Warm Tape", "Tape & Dub", pv::warmTape, 0}, {"Worn Cassette", "Tape & Dub", pv::cassette, 0},
    {"Rolling Tape", "Tape & Dub", pv::rolling, 0}, {"Runaway Dub", "Tape & Dub", pv::runaway, 0}, {"Lo-Fi Slap", "Tape & Dub", pv::lofiSlap, 0}, {"Dub Space", "Tape & Dub", pv::dubSpace, 0},
    {"Chorus", "Modulation", pv::chorus, 0}, {"Wide Chorus", "Modulation", pv::wideChorus, 0}, {"Flanger", "Modulation", pv::flanger, 0}, {"Jet Flanger", "Modulation", pv::jet, 0},
    {"Negative Flanger", "Modulation", pv::negFlanger, 0}, {"Phaser", "Modulation", pv::phaser, 0}, {"Vibrato", "Modulation", pv::vibrato, 0}, {"Rotary", "Modulation", pv::rotary, 0},
    {"Sweeping Echo", "Filtered", pv::sweepEcho, 0}, {"Darkening", "Filtered", pv::darkening, 0}, {"Telephone", "Filtered", pv::telephone, 0},
    {"Resonant Taps", "Filtered", pv::resoTaps, 0}, {"Acid Echo", "Filtered", pv::acidEcho, 0}, {"Envelope Opens", "Filtered", pv::envOpens, 0}, {"Ducked Echo", "Filtered", pv::ducked, 0},
    {"Scratch", "FX & Scratch", pv::scratch, 0}, {"Tape Dive", "FX & Scratch", pv::tapeDive, 0}, {"Stretch Stutter", "FX & Scratch", pv::stutter, 0},
    {"Haze", "FX & Scratch", pv::haze, 0}, {"Infinite Loop", "FX & Scratch", pv::loop, 0}, {"Wheel Feedback", "FX & Scratch", pv::wheelFb, 0},
    {"Key Comb", "FX & Scratch", pv::keyComb, 0}, {"Bend the Tape", "FX & Scratch", pv::bendTime, 0},
    // 3.0
    {"Tresillo Taps", "Multi-Tap", pv::tapTres, 6}, {"Golden Taps", "Multi-Tap", pv::tapGolden, 10}, {"Tap Cloud", "Multi-Tap", pv::tapCloud, 11},
    {"Swell Into Echo", "Multi-Tap", pv::tapSwell, 12}, {"Sixteenth Ramp", "Multi-Tap", pv::tapRamp, 4},
    {"Shimmer", "Space & Pitch", pv::shimmer, 0}, {"Octave Down Dub", "Space & Pitch", pv::octDown, 0}, {"Fifth Cascade", "Space & Pitch", pv::fifths, 0},
    {"Diffused Wash", "Space & Pitch", pv::wash, 0}, {"Lo-Fi Memory", "Space & Pitch", pv::memory, 0}, {"Ducking Echo", "Space & Pitch", pv::duck3, 0},
    {"Swelling Echo", "Space & Pitch", pv::swell3, 0}, {"Freeze Pad (Wheel)", "Space & Pitch", pv::freezePad, 0}, {"Mid/Side Echo", "Space & Pitch", pv::msWide, 0},
    {"Six-Band Sculpt", "Filtered", pv::sixBand, 0}, {"Step Pan Echo", "Modulation", pv::stepPan, 0}, {"Sidechain Duck", "Space & Pitch", pv::scDuck, 0},
};
static constexpr uint32_t kNumPresets = sizeof(kPresetDefs) / sizeof(kPresetDefs[0]);

inline const Preset* presets() {
    static Preset table[kNumPresets];
    static bool done = false;
    if (done) return table;
    done = true;
    Engine e;
    for (uint32_t i = 0; i < kNumPresets; ++i) {
        table[i].name = kPresetDefs[i].name; table[i].category = kPresetDefs[i].category;
        std::copy(e.p, e.p + kParamCount, table[i].v);
        if (kPresetDefs[i].taps > 0) { const TapPattern& t = tapFactory()[kPresetDefs[i].taps - 1]; for (int k = 0; k < kTapPatternFields; ++k) table[i].v[tapPatternParam(k)] = t.v[k]; }
        for (const PV* x = kPresetDefs[i].pv; x->id != kParamCount; ++x) table[i].v[x->id] = x->v;
    }
    return table;
}

} // namespace strata
