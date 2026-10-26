// Strata — hover tips and the in-plugin help pages. License: MIT.
#pragma once
#include "ParamDefs.hpp"

namespace strata {

static const char* const kManualURL = "https://github.com/natehouk/strata/blob/main/docs/MANUAL.md";

inline const char* styleTip(int s) {
    switch (s) {
    case 0: return "Smooth: gentle and broad, with a soft, wide resonance.";
    case 1: return "Warm: a saturated input into a linear filter. Rounded, never harsh.";
    case 2: return "Monolith: the Monolith synth's filter. Soft-limited, and it self-oscillates at high Peak.";
    case 3: return "Hot: saturating integrators. A thick sound with a screaming peak.";
    case 4: return "Acid: resonance thins out the bass. Squelchy and rubbery.";
    case 5: return "Raw: hard-clipped feedback. Buzzy, aggressive self-oscillation.";
    case 6: return "Clean: linear and transparent, never self-oscillates. The style for precise EQ in the loop.";
    case 7: return "Tube: an asymmetric, valve-like input stage. Warm overdrive with mostly even harmonics.";
    case 8: return "Gentle: clean, polite and broad. Never self-oscillates.";
    case 9: return "Metal: a hard-clipped input and hard-clipped feedback. Rough and distorted; each repeat gets grittier.";
    case 10: return "Hollow: the band at the cutoff is carved out as the peak rises, for a nasal tone.";
    default: return "";
    }
}
inline const char* filterFieldTip(int k) {
    switch (k) {
    case kFOn: return "Switches this filter on or off. All six filters sit inside the feedback loop, so every repeat is filtered again.";
    case kFStyle: return "The filter's character, from Clean to Metal. Click to choose.";
    case kFType: return "LP darkens each repeat, BP keeps a band, HP thins it. BELL boosts or cuts around the cutoff; LO SH and HI SH boost or cut below or above it; NOTCH removes a narrow band. GAIN sets the boost or cut.";
    case kFSlope: return "How steep the filter is: 6, 12, 24 or 48 dB per octave. For bell and shelf shapes, steeper means narrower.";
    case kFFreq: return "The cutoff (or centre) frequency, 20 Hz to 20 kHz.";
    case kFPeak: return "Resonance: echoes ring at the cutoff as they repeat. For bell, shelf and notch shapes it narrows the band instead.";
    case kFPan: return "Offsets the cutoff between the left and right lines.";
    case kFGain: return "For bell and shelf shapes: how much to boost or cut, up to 24 dB. The other shapes ignore it.";
    default: return "";
    }
}

inline const char* paramTip(uint32_t p) {
    for (int f = 0; f < kNumFilters; ++f) for (int k = 0; k <= kFGain; ++k) if (fpar(f, k) == p) return filterFieldTip(k);
    for (int t = 0; t < kNumTaps; ++t) {
        if (p == tpar(t, kTOn)) return "Switches this extra tap on or off. Each tap is another read head on both delay lines; it adds an echo inside every repeat without being fed back.";
        if (p == tpar(t, kTTime)) return "Where this tap sits, as a fraction of the delay time (1% to 100%). It follows the delay: change the time and the whole pattern scales.";
        if (p == tpar(t, kTLevel)) return "This tap's level, -60 to +6 dB.";
        if (p == tpar(t, kTPan)) return "Pans this tap: left takes it from the left line only, right from the right line only.";
    }
    switch (p) {
    case kChanMode: return "LEFT/RIGHT: the two lines carry the left and right channels. MID/SIDE: the left line carries the mid (centre) and the right line the side (stereo difference); the wet signal is decoded back to left and right.";
    case kFreeze: return "Holds what is in the delay lines and loops it forever: the input is muted, feedback, filters and effects are bypassed, and the buffer plays on untouched. Release to continue.";
    case kWidth: return "Stereo width of what enters the delay lines: 0% is mono, 100% unchanged. In MID/SIDE mode it scales the side line.";
    case kWetLevel: return "Level of the echoes, before the mix, -36 to +12 dB.";
    case kWetPan: return "Balance of the echoes.";
    case kLockMix: return "Keeps the MIX where it is when you load a preset: useful on an insert, or on a send at 100%.";
    case kLofiOn: case kLofi: return "LO-FI in the loop: lower sample rate and bit depth, darker with every repeat. The knob sets how much.";
    case kDiffOn: case kDiffuse: return "DIFFUSE in the loop: chains of all-pass filters smear each repeat, from soft echoes to a reverb-like wash.";
    case kDynOn: case kDyn: return "DYNAMICS follows the input level. Left of centre ducks: the echoes dip while you play and come back up in the gaps. Right of centre swells: the input to the lines is evened out, quiet parts up and loud parts down.";
    case kPitchOn: return "Switches the pitch shifter on. In the loop, every repeat shifts again: cascading arpeggios and shimmer.";
    case kPitch: return "Pitch shift, -12 to +12 semitones. The shifter is grain based (40 ms grains): smooth on pads, grainy on transients.";
    case kPitchMirror: return "The right line shifts the other way: +7 on the left becomes -7 on the right.";
    case kPitchInLoop: return "On: the shifter sits inside the feedback loop and each repeat shifts again. Off: only the output is shifted, so every repeat has the same pitch.";
    case kMainOn: return "The main tap: the echo at the full delay time, the only one that is fed back. Switch it off to hear only the extra taps (the feedback still runs).";
    case kMainLevel: return "Level of the main tap, -60 to +6 dB.";
    case kMainPan: return "Pans the main tap.";
    default: break;
    }
    if (p >= kXlfoFirst && p < kEgFirst) {
        switch ((p - kXlfoFirst) % kXlfoFields) {
        case em::kLMode: return "WAVE: a classic LFO shape. STEPS: a 16-step sequencer you draw in the step display.";
        case em::kLWave: return "The LFO shape: sine, triangle, saw up, saw down, square, smooth random, sample & hold.";
        case em::kLRate: return "LFO speed in Hz when not synced.";
        case em::kLSync: return "FREE runs at the Rate. SYNC, DOTTED and TRIPLET lock to the host tempo and follow the song position.";
        case em::kLDiv: return "Length of one cycle when synced, from 16 bars down to a 1/64 note.";
        case em::kLBalance: return "Skews the shape: for triangle and sine from falling to rising ramp; for square it sets the duty cycle.";
        case em::kLGlide: return "In step mode, how smoothly the value slides from one step to the next.";
        case em::kLSteps: return "How many steps the sequence uses before it repeats.";
        default: return "One step of the sequence. Drag up and down in the step display.";
        }
    }
    if (p >= kEgFirst && p < kEfFirst) {
        switch ((p - kEgFirst) % kEgFields) {
        case em::kEAttack: return "Envelope attack time."; case em::kEDecay: return "Envelope decay time."; case em::kESustain: return "Level the envelope holds while triggered.";
        case em::kERelease: return "Envelope release time.";
        case em::kETrigger: return "MIDI: the envelope starts with each note. AUDIO: it starts when the input rises above the threshold.";
        default: return "Input level that triggers the envelope in AUDIO mode.";
        }
    }
    if (p >= kEfFirst && p < kXyFirst) {
        switch ((p - kEfFirst) % kEfFields) {
        case em::kFAttack: return "How fast the follower rises with the level."; case em::kFRelease: return "How fast it falls back.";
        case em::kFGain: return "Sensitivity: raises or lowers the level the follower sees.";
        default: return "INPUT follows the plugin's input. SIDECHAIN follows the sidechain input (route another track to it in your DAW).";
        }
    }
    if (p >= kXyFirst && p < kSlot2First) return "XY controller position. Drag the dot in the pad; X and Y are two separate sources.";
    if (p >= kSlot2First && p < kParamCount) {
        switch ((p - kSlot2First) % kSlotFields) {
        case kSlotSrc: return "The modulation source. Click to change.";
        case kSlotDst: return "What this slot modulates. Click to change: every knob of Strata, the taps, the effects, the sources' rates and times, and other slots' amounts.";
        case kSlotAmt: return "How much, and in which direction. Turn left to invert. On a delay time, small amounts give tape wow and chorus.";
        default: return "Switches this slot off without deleting it.";
        }
    }
    if (p >= kSlotFirst && p <= kSlotLast) {
        switch ((p - kSlotFirst) % 4) {
        case kSlotSrc: return "What drives this modulation slot. Click to pick a source; an empty slot does nothing.";
        case kSlotDst: return "What this slot modulates. Click to pick a destination: delay times, pans, feedback, drive, filters, levels, LFOs, the envelope, or another slot's amount.";
        case kSlotAmt: return "How much the source moves the destination. Turn left to invert. Delay time reaches 5 octaves: tiny amounts give tape wow and chorus.";
        default: return "Bypasses this slot without losing its settings.";
        }
    }
    switch (p) {
    case kSyncL: case kSyncR: return "FREE sets the delay in milliseconds. STR, DOT and TRIP lock it to the host tempo as a straight, dotted or triplet note; the TIME knob then picks the note.";
    case kTimeL: case kTimeR: return "The delay time, 1 ms to 4 s. When synced, this knob picks a note length (1/64 to 2 bars). In TAPE mode, turning it quickly bends the pitch: scratching.";
    case kDivL: case kDivR: return "The delay as a note length, from 1/64 to 2 bars.";
    case kOffL: case kOffR: return "Pushes a synced echo up to 50% earlier or later than the grid, for swing and push.";
    case kPanL: case kPanR: return "Where this side's echoes sit in the stereo field.";
    case kFbL: case kFbR: return "How much of this side's output is fed back into it: more feedback, more repeats. Above 100% it builds up, held in check by DRIVE.";
    case kXfLR: return "How much of the left echo is fed into the right line. Use with RIGHT's cross-feedback for bouncing echoes.";
    case kXfRL: return "How much of the right echo is fed into the left line.";
    case kInvL: case kInvR: return "Flips the polarity of what this side feeds back. With short times, a flanger becomes a hollow, negative flanger.";
    case kLink: return "Makes the right side follow the left side's time, sync and offset.";
    case kMode: return "TAPE: a time change speeds the read head up or slows it down, so the pitch bends like real tape. STRETCH: a time change crossfades to the new time, without any pitch change.";
    case kGlide: return "TAPE mode: how fast the head glides to a new time. STRETCH mode: how long the crossfade takes.";
    case kDrive: return "Tape-style saturation inside the feedback loop: each repeat gets a little dirtier, and feedback above 100% stays under control.";
    case kInput: return "STEREO: each input channel feeds its own line. MONO: the sum feeds both. P-P L / P-P R (ping-pong): the sum enters the left (or right) line only and bounces across through cross-feedback.";
    case kF1On: case kF2On: return "Switches this filter on or off. Both filters sit inside the feedback loop, so every repeat is filtered again.";
    case kF1Style: case kF2Style: return "The filter's character: SMOOTH, WARM, MONOLITH (the Monolith synth's filter), HOT, ACID or RAW. Click the arrows or scroll.";
    case kF1Type: case kF2Type: return "LP darkens each repeat, HP thins it, BP keeps a band around the cutoff.";
    case kF1Slope: case kF2Slope: return "How steeply the filter cuts: 12, 24 or 48 dB per octave.";
    case kF1Freq: case kF2Freq: return "The cutoff frequency, 20 Hz to 20 kHz.";
    case kF1Peak: case kF2Peak: return "Resonance: emphasises the cutoff. Echoes ring at that frequency as they repeat.";
    case kF1Pan: case kF2Pan: return "Offsets the cutoff between the left and right lines.";
    case kRouting: return "SERIAL: the loop goes through every filter that is on, 1 to 6. PARALLEL: each filter processes the loop and the results are mixed. PER CHANNEL: filters 1, 3 and 5 sit in the left (or mid) line, 2, 4 and 6 in the right (or side) line.";
    case kInGain: return "The level into the delay lines, up to 24 dB either way. Hotter input drives the saturating filter styles and DRIVE.";
    case kMix: return "Dry/wet balance. On a send, use 100%.";
    case kOutGain: return "Output level, up to 24 dB either way.";
    case kL1Wave: case kL2Wave: return "Classic LFO shape: TRI, SIN, SQR, RND (glides between random values) or S&H (jumps between random values).";
    case kL1Balance: case kL2Balance: return "For triangle and sine, skews the shape from a falling ramp through symmetric (50%) to a rising ramp. For square, sets the duty cycle.";
    case kL1Sync: case kL2Sync: return "FREE runs at RATE in Hz. STR, DOT and TRIP lock the LFO to the host tempo and song position; RATE then picks the note length.";
    case kL1Rate: case kL2Rate: return "LFO speed, 0.01 to 50 Hz. When synced, this knob picks a note length instead.";
    case kL1Div: case kL2Div: return "The LFO cycle length when synced.";
    case kL1NoteSync: case kL2NoteSync: return "Restarts the LFO on every MIDI note.";
    case kEgAttack: return "Envelope attack: how long it takes to rise once triggered.";
    case kEgDecay: return "Envelope decay: how long it takes to fall to the sustain level.";
    case kEgSustain: return "Envelope sustain: the level it holds while triggered.";
    case kEgRelease: return "Envelope release: how long it takes to fall back to zero.";
    case kEgThreshold: return "The input level that triggers the envelope (AUDIO trigger). It starts when the input rises past this, and releases when it falls below.";
    case kEgTrigger: return "AUDIO: the input level triggers the envelope, like an envelope follower. MIDI: MIDI notes trigger it.";
    case kMidiOn: return "Turns all MIDI handling on or off: MIDI sources, triggers, retrigger and MIDI Learn. Switch it off in hosts that send MIDI to every effect.";
    default: return "";
    }
}

enum TipTarget { kTipUndo, kTipRedo, kTipLearn, kTipMidi, kTipHelp, kTipA, kTipB, kTipCopy, kTipPrev, kTipPresets, kTipNext,
                 kTipChip, kTipSlotDel, kTipEchoes, kTipTap, kTipClear, kTipFilter, kTipEnv, kTipLfo,
                 kTipTapEdit, kTipView, kTipTapPatterns, kTipFilterTab, kTipStyle, kTipStepEdit, kTipXyPad, kTipSourceTab, kTipCompPresets,
                 kTipModRing, kTipClassicChip, kTipClassicRow, kNumTipTargets };
inline const char* targetTitle(int t) {
    static const char* const n[kNumTipTargets] = { "Undo", "Redo", "MIDI Learn", "MIDI", "Help", "Setting A", "Setting B", "Copy A/B",
                                                   "Previous preset", "Presets", "Next preset", "Modulation source", "Remove slot",
                                                   "Echo display", "Tap tempo", "Clear echoes", "Filter response", "Envelope", "LFO",
                                                   "Tap editor", "Display view", "Tap patterns", "Filter", "Style", "Step sequence", "XY pad",
                                                   "Modulation source", "Source presets", "Modulated", "Classic source", "Classic modulation" };
    return n[t];
}
inline const char* targetTip(int t) {
    switch (t) {
    case kTipUndo: return "Steps back through your edits, including new modulation, tap tempo, preset loads and A/B switches. Cmd/Ctrl-Z.";
    case kTipRedo: return "Re-applies an edit you undid. Cmd/Ctrl-Shift-Z or Cmd/Ctrl-Y.";
    case kTipLearn: return "Click, then click any control and move a knob or fader on your MIDI controller to assign it. Right-click a control to unassign.";
    case kTipMidi: return paramTip(kMidiOn);
    case kTipHelp: return "Opens the help pages: delay lines, taps, tape and stretch, filters, effects, modulation, MIDI and shortcuts.";
    case kTipA: case kTipB: return "Strata keeps two complete settings, A and B. Click the other letter to switch and compare.";
    case kTipCopy: return "Copies the current setting to the other slot.";
    case kTipPrev: return "Load the previous preset. The mouse wheel over the preset name also steps through presets.";
    case kTipPresets: return "Open the preset browser: factory presets in eight categories, your own presets, and SAVE CURRENT AS. A * after the name means you have changed it. LOCK MIX (in OUTPUT) keeps the mix when you load one. In the browser, type to search names and categories, and click a star to keep a preset in Favourites.";
    case kTipNext: return "Load the next preset.";
    case kTipChip: return "Drag this source onto any knob to modulate it. A slot is created with a starting amount, and the knob gets a coloured ring.";
    case kTipSlotDel: return "Removes this modulation slot.";
    case kTipEchoes: return "Every repeat of a single hit on both lines, including feedback, cross-feedback, the loop filters at 1 kHz and the extra taps (thin lines). Hollow dots are inverted repeats. Updates live as the times move.";
    case kTipTap: return "Click in time to set the delay. The left side takes the tapped time; the right side keeps its ratio to the left.";
    case kTipClear: return "Silences everything still echoing in the delay lines. MIDI CC 120 (All Sound Off) does the same.";
    case kTipFilter: return "The response of the filters in the loop (the selected one highlighted) and, in white, their combined curve for the current routing.";
    case kTipEnv: return "The envelope's shape. Drag its chip (top right) onto a knob to use it.";
    case kTipLfo: return "The LFO's shape. Drag its chip (top right) onto a knob to use it.";
    case kTipTapEdit: return "The tap pattern across one delay time (the main tap is the tall bar at the right). Double-click an empty spot to add a tap. Drag a tap: left/right is its time, up/down its level. Scroll over a tap to pan it. Right-click a tap to switch it off; right-click the main tap to switch it on or off.";
    case kTipView: return "MAP shows every repeat of a single hit, taps included. TAPS edits the tap pattern.";
    case kTipTapPatterns: return "Load a ready-made tap pattern (the main tap and all 15 extra taps) without touching the rest of the patch, or save your own. Right-click a saved one to delete it.";
    case kTipFilterTab: return "Click to edit this filter. The dot switches it on or off.";
    case kTipStyle: return "Click to choose the filter's style.";
    case kTipStepEdit: return "Draw the sequence: click or drag up and down across the steps. Cmd/Ctrl-click resets a step to zero.";
    case kTipXyPad: return "Drag the dot. X and Y are separate modulation sources; drag their chips onto knobs.";
    case kTipSourceTab: return "Choose a modulation source to edit: four XLFOs (wave or step sequencer), two envelopes, two envelope followers, two XY pads, the MIDI sources, and CLASSIC (the two LFOs and the envelope of Strata 2).";
    case kTipCompPresets: return "Presets for just this source: load a ready-made LFO shape, sequence, envelope or follower setting without touching the rest of the patch, or save your own. Right-click a saved one to delete it.";
    case kTipModRing: return "This knob is modulated. Click the ring (or right-click the knob) to list every slot that moves it, with its amount, bypass and delete.";
    case kTipClassicChip: return "A classic source from Strata 2. Drag it onto a knob that Strata 2 could modulate; it creates a classic slot (up to 24). For everything else, use the XLFOs, envelopes and followers.";
    case kTipClassicRow: return "A classic slot (Strata 2's modulation). Click the source or destination to change it; drag the amount.";
    default: return "";
    }
}

struct HelpPage { const char* title; const char* body; };
static const HelpPage kHelpPages[] = {
    { "Overview",
      "Strata is a stereo tape delay: two delay lines with feedback and cross-feedback, 15 extra taps, six filters and four effects inside the loop, freeze, and drag-and-drop modulation.\n\n"
      "1. Set each side's TIME (or sync it to the tempo) and FEEDBACK.\n"
      "2. Pick TAPE (pitch bends when the time changes) or STRETCH (it doesn't).\n"
      "3. Shape the repeats with the filters and EFFECTS.\n"
      "4. Add taps in the TAPS view for rhythmic patterns.\n"
      "5. Drag a source chip onto any knob to modulate it.\n\n"
      "Hover over anything for a moment to see what it does." },
    { "Delay lines",
      "Each side has TIME (1 ms to 4 s), or a synced note (straight, dotted, triplet) with OFFSET for swing. PAN, FEEDBACK and CROSS feed the echoes back into the same or the other side. INVERT flips the polarity of the feedback.\n\n"
      "LINK makes the right side follow the left. TAP sets the time from your clicks. FREEZE holds the buffer and loops it.\n\n"
      "INPUT: STEREO, MONO, or ping-pong into the left (P-P L) or right (P-P R) line only. CHANNELS (in OUTPUT): LEFT/RIGHT or MID/SIDE lines; WIDTH narrows what goes in." },
    { "Taps",
      "Besides the main tap (the echo at the full delay time, the one that is fed back), each line has 15 extra taps. Each sits at a fraction of the delay time, with its own level and pan, so a pattern follows the delay as it changes.\n\n"
      "Switch the display to TAPS: double-click to add a tap, drag it (left/right time, up/down level), scroll over it to pan, right-click to remove. PATTERNS loads and saves whole tap patterns.\n\n"
      "Taps are output only: they add echoes inside each repeat without building up." },
    { "Tape & filters",
      "TAPE mode: changing the time bends the pitch like real tape; turn TIME quickly to scratch. STRETCH mode: changes crossfade without any pitch change. SMOOTH sets how fast either happens.\n\n"
      "Six filters sit in the loop, so each repeat is filtered again: 11 styles; low/band/high pass, bell, shelves and notch; 6 to 48 dB/oct. ROUTING: serial, parallel, or per channel (odd filters left, even right)." },
    { "Effects",
      "DRIVE saturates inside the loop: each repeat gets dirtier.\n"
      "LO-FI reduces the sample rate and bit depth of the loop.\n"
      "DIFFUSE smears each repeat, up to a reverb-like wash.\n"
      "DYNAMICS ducks the echoes while you play (left) or swells quiet input (right).\n"
      "PITCH shifts by up to an octave. In the loop, each repeat shifts again; MIRROR shifts the right line the other way.\n\n"
      "WET LEVEL and WET PAN set the echoes before the MIX." },
    { "Modulation",
      "Pick a source tab: four XLFOs (wave or 16-step sequencer), two envelopes (MIDI or audio triggered), two envelope followers (input or sidechain), two XY pads, the MIDI sources, and CLASSIC (Strata 2's two LFOs and envelope).\n\n"
      "Drag a source's chip onto any knob. The knob gets a coloured ring, and the connection appears in the list (up to 50, plus 24 classic). Right-click a knob to see what moves it. Drop a chip on a slot's amount to modulate the amount.\n\n"
      "The PRESETS menu in each source loads or saves just that source." },
    { "MIDI & shortcuts",
      "MIDI LEARN: click LEARN, click a control, move a controller. Right-click unassigns. The MIDI button switches all MIDI handling off.\n\n"
      "A/B holds two complete settings; COPY copies the current one to the other slot. Every edit is an undo step (Cmd/Ctrl-Z, Cmd/Ctrl-Shift-Z).\n\n"
      "Double-click a knob to type a value; Cmd/Ctrl-click resets it; Shift-drag is fine adjustment; grab the rim of a large knob to turn it in a circle. Esc closes this window." },
};
static constexpr int kNumHelpPages = sizeof(kHelpPages) / sizeof(kHelpPages[0]);

} // namespace strata
