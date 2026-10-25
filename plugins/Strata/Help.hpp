// Strata — hover tips and the in-plugin help pages. License: MIT.
#pragma once
#include "ParamDefs.hpp"

namespace strata {

static const char* const kManualURL = "https://github.com/natehouk/strata/blob/main/docs/MANUAL.md";

inline const char* paramTip(uint32_t p) {
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
    case kInput: return "STEREO: each input channel feeds its own line. MONO: the sum feeds both. PING-PONG: the sum enters the left line only and bounces across through cross-feedback.";
    case kF1On: case kF2On: return "Switches this filter on or off. Both filters sit inside the feedback loop, so every repeat is filtered again.";
    case kF1Style: case kF2Style: return "The filter's character: SMOOTH, WARM, MONOLITH (the Monolith synth's filter), HOT, ACID or RAW. Click the arrows or scroll.";
    case kF1Type: case kF2Type: return "LP darkens each repeat, HP thins it, BP keeps a band around the cutoff.";
    case kF1Slope: case kF2Slope: return "How steeply the filter cuts: 12, 24 or 48 dB per octave.";
    case kF1Freq: case kF2Freq: return "The cutoff frequency, 20 Hz to 20 kHz.";
    case kF1Peak: case kF2Peak: return "Resonance: emphasises the cutoff. Echoes ring at that frequency as they repeat.";
    case kF1Pan: case kF2Pan: return "Offsets the cutoff between the left and right lines.";
    case kRouting: return "SERIAL: the loop goes through filter 1, then filter 2. PARALLEL: both filters process the loop and are mixed. Only matters when both filters are on.";
    case kInGain: return "The level into the delay lines, up to 24 dB either way. Hotter input drives the saturating filter styles and DRIVE.";
    case kMix: return "Dry/wet balance. On a send, use 100%.";
    case kOutGain: return "Output level, up to 24 dB either way.";
    case kL1Wave: case kL2Wave: return "The LFO's shape: TRI, SIN, SQR, RND (glides between random values) or S&H (jumps between random values).";
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
                 kTipChip, kTipSlotDel, kTipEchoes, kTipTap, kTipClear, kTipFilter, kTipEnv, kTipLfo, kNumTipTargets };
inline const char* targetTitle(int t) {
    static const char* const n[kNumTipTargets] = { "Undo", "Redo", "MIDI Learn", "MIDI", "Help", "Setting A", "Setting B", "Copy A/B",
                                                   "Previous preset", "Presets", "Next preset", "Modulation source", "Remove slot",
                                                   "Echo display", "Tap tempo", "Clear echoes", "Filter response", "Envelope", "LFO" };
    return n[t];
}
inline const char* targetTip(int t) {
    switch (t) {
    case kTipUndo: return "Steps back through your edits, including new modulation, tap tempo, preset loads and A/B switches. Cmd/Ctrl-Z.";
    case kTipRedo: return "Re-applies an edit you undid. Cmd/Ctrl-Shift-Z or Cmd/Ctrl-Y.";
    case kTipLearn: return "Click, then click any control and move a knob or fader on your MIDI controller to assign it. Right-click a control to unassign.";
    case kTipMidi: return paramTip(kMidiOn);
    case kTipHelp: return "Opens the help pages: delay lines, tape and stretch, filters, modulation, MIDI and shortcuts.";
    case kTipA: case kTipB: return "Strata keeps two complete settings, A and B. Click the other letter to switch and compare.";
    case kTipCopy: return "Copies the current setting to the other slot.";
    case kTipPrev: return "Load the previous preset. The mouse wheel over the preset name also steps through presets.";
    case kTipPresets: return "Open the preset browser: factory presets in six categories, your own presets, and SAVE CURRENT AS. A * after the name means you have changed it.";
    case kTipNext: return "Load the next preset.";
    case kTipChip: return "Drag this source onto any knob to modulate it. A slot is created with a starting amount, and the knob gets a coloured ring.";
    case kTipSlotDel: return "Removes this modulation slot.";
    case kTipEchoes: return "Every repeat of a single hit on both lines, including feedback, cross-feedback and the loop filters. Hollow dots are inverted repeats. Updates live as the times move.";
    case kTipTap: return "Click in time to set the delay. The left side takes the tapped time; the right side keeps its ratio to the left.";
    case kTipClear: return "Silences everything still echoing in the delay lines. MIDI CC 120 (All Sound Off) does the same.";
    case kTipFilter: return "The filter's response at the current type, slope, cutoff and peak.";
    case kTipEnv: return "The envelope's shape. Drag its chip (top right) onto a knob to use it.";
    case kTipLfo: return "The LFO's shape. Drag its chip (top right) onto a knob to use it.";
    default: return "";
    }
}

struct HelpPage { const char* title; const char* body; };
static const HelpPage kHelpPages[] = {
    { "Overview",
      "Strata is a stereo tape delay: two delay lines with feedback and cross-feedback, two filters inside the loop, and drag-and-drop modulation.\n\n"
      "1. Set each side's TIME (or sync it to the tempo) and FEEDBACK.\n"
      "2. Pick TAPE (pitch bends when the time changes) or STRETCH (it doesn't).\n"
      "3. Shape the repeats with the loop filters and DRIVE.\n"
      "4. Drag a source chip onto any knob to modulate it.\n\n"
      "Hover over anything for a moment to see what it does." },
    { "Delay lines",
      "Each side has TIME (1 ms to 4 s), or a synced note (straight, dotted, triplet) with OFFSET for swing. PAN, FEEDBACK and CROSS feed the echoes back into the same or the other side. INVERT flips the polarity of the feedback.\n\n"
      "LINK makes the right side follow the left. TAP TEMPO sets the time from your clicks.\n\n"
      "INPUT: STEREO, MONO, or PING-PONG (into the left line only, bouncing through cross-feedback)." },
    { "Tape & filters",
      "TAPE mode: changing the time bends the pitch like real tape; turn TIME quickly to scratch. STRETCH mode: changes crossfade without any pitch change. SMOOTH sets how fast either happens.\n\n"
      "DRIVE saturates inside the loop: each repeat gets dirtier.\n\n"
      "Two filters sit in the loop, so each repeat is filtered again: six styles, LP/BP/HP at 12, 24 or 48 dB, serial or parallel." },
    { "Modulation",
      "Drag a source chip (LFO 1, LFO 2, ENV, VEL, AT, KEY, BEND, WHEEL) onto any knob. The knob gets a coloured ring and a slot appears in the MODULATION panel.\n\n"
      "Each slot has a source, a destination, an amount (left of centre inverts), bypass and delete. Every slot's amount is itself a destination.\n\n"
      "The envelope is triggered by the input level (AUDIO) or by MIDI notes. Small amounts on TIME give tape wow and chorus." },
    { "MIDI & shortcuts",
      "MIDI LEARN: click LEARN, click a control, move a controller. Right-click unassigns. The MIDI button switches all MIDI handling off.\n\n"
      "A/B holds two complete settings; COPY copies the current one to the other slot. Every edit is an undo step (Cmd/Ctrl-Z, Cmd/Ctrl-Shift-Z).\n\n"
      "Double-click a knob to type a value; Cmd/Ctrl-click resets it; Shift-drag is fine adjustment; grab the rim of a large knob (TIME, FEEDBACK, FREQ, MIX) to turn it in a circle. Esc closes this window." },
};
static constexpr int kNumHelpPages = sizeof(kHelpPages) / sizeof(kHelpPages[0]);

} // namespace strata
