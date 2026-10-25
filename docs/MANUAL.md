# Strata 2.0 — User Manual

Strata is a stereo tape delay. It has:

- two delay lines, each with feedback and cross-feedback
- tape-style or pitch-free time changes
- two filters inside the feedback loop
- two LFOs and an envelope, with 24 drag-and-drop modulation slots

Typical uses:

- echoes, slapback and ping-pong delays, in time with the song
- dub and tape delays that bend, saturate and run away
- chorus, flanger, phaser and vibrato
- scratching and other time effects

Strata runs as VST3, AU, CLAP and VST2 on macOS, Windows and Linux.

**Contents**

1. [Installing](#1-installing)
2. [A quick tour](#2-a-quick-tour)
3. [The delay lines](#3-the-delay-lines)
4. [Tape and stretch](#4-tape-and-stretch)
5. [The echo display](#5-the-echo-display)
6. [The loop filters](#6-the-loop-filters)
7. [The LFOs and the envelope](#7-the-lfos-and-the-envelope)
8. [Modulation](#8-modulation)
9. [Output](#9-output)
10. [Knobs](#10-knobs)
11. [A/B, undo and redo](#11-ab-undo-and-redo)
12. [MIDI](#12-midi)
13. [Presets](#13-presets)
14. [Help and hover tips](#14-help-and-hover-tips)
15. [Recipes](#15-recipes)
16. [Parameter reference](#16-parameter-reference)
17. [Troubleshooting](#17-troubleshooting)

---

## 1. Installing

Download the zip for your system from the project's **Releases** page and copy the plugin files:

| System | Copy | To |
|---|---|---|
| macOS | `Strata.vst3`, `Strata.component` (AU), `Strata.clap` | `~/Library/Audio/Plug-Ins/VST3`, `.../Components`, `.../CLAP` |
| Windows | the `Strata.vst3` folder, `Strata.clap` | `C:\Program Files\Common Files\VST3\`, `...\CLAP\` |
| Linux | `Strata.vst3`, `Strata.clap` | `~/.vst3/`, `~/.clap/` |

**macOS:** if the build isn't notarized and macOS blocks it, open Terminal and run:

```
xattr -dr com.apple.quarantine ~/Library/Audio/Plug-Ins/*/Strata*
```

Rescan plugins in your DAW. Strata appears under **Epsilon Audio** as an effect.

Strata also accepts MIDI. To use the MIDI sources (velocity, key, wheel and so on) or the MIDI-triggered envelope, route a MIDI track to it; how depends on your DAW.

Projects saved with Strata 1.x open unchanged in 2.0.

---

## 2. A quick tour

![Strata](../screenshot.png)

- **Header:** the preset browser, **A / B** and **COPY**, **UNDO / REDO**, **LEARN** (MIDI Learn), **MIDI** on/off and **HELP**.
- **Top row:**
  - **LEFT** and **RIGHT:** each delay line's sync mode, TIME, OFFSET, PAN, FEEDBACK, CROSS and INVERT. RIGHT also has **LINK**.
  - **ECHOES:** the echo display, MODE (TAPE or STRETCH), SMOOTH, DRIVE, INPUT and **TAP TEMPO**.
- **Middle row:** **FILTER 1**, **FILTER 2**, **LFO 1**, **LFO 2** and **ENVELOPE**. Each LFO and the envelope have a coloured source chip at the top right.
- **Bottom row:**
  - **MODULATION:** chips for velocity, aftertouch, key, pitch bend and mod wheel, and the 24 slots
  - **OUTPUT:** filter routing, INPUT, MIX, OUTPUT and **CLEAR ECHOES**

---

## 3. The delay lines

Strata has a left and a right delay line. Each one has:

- **Sync mode:**
  - **FREE:** **TIME** sets the delay in milliseconds, 1 ms to 4 s.
  - **STRAIGHT**, **DOTTED**, **TRIPLET:** the delay is a note length locked to the host tempo, from 1/64 to 2 bars. TIME then picks the note.
- **OFFSET** (±50%): pushes a synced echo earlier or later than the grid, for swing. It is greyed out in FREE.
- **PAN:** where the side's echoes sit.
- **FEEDBACK:** how much of the side's output feeds back into it. More feedback, more repeats; above 100% the echoes build up, held in check by DRIVE.
- **CROSS:** how much of this side's echo feeds the other side.
- **INVERT:** flips the polarity of what the side feeds back. With short times, a flanger becomes a hollow, negative flanger.

The live time of each side is shown beside its title.

**LINK** (on RIGHT) makes the right side follow the left side's time, sync and offset.

**TAP TEMPO:** click in time. The left side takes the tapped time; the right side keeps its ratio to the left.

**INPUT:**
- **STEREO:** each input channel feeds its own line.
- **MONO:** the sum feeds both lines.
- **PING-PONG:** the sum enters the left line only and bounces across through cross-feedback.

---

## 4. Tape and stretch

**MODE** decides what happens when a delay time changes:

- **TAPE:** the read head speeds up or slows down, so the pitch bends like real tape. Turn TIME quickly, or modulate it, to **scratch**.
- **STRETCH:** the delay crossfades to the new time, so the pitch never changes.

**SMOOTH** sets how fast the change happens: the head's glide in TAPE mode, the crossfade length in STRETCH mode.

**DRIVE** adds tape-style saturation inside the loop. Each repeat gets a little dirtier.

---

## 5. The echo display

The display draws every repeat of a single hit on both lines:

- left echoes above the centre line, right echoes below
- feedback, cross-feedback and the loop filters are all taken into account
- hollow dots are inverted repeats

It updates live as the times are modulated.

---

## 6. The loop filters

Both filters sit **inside the feedback loop**, so every repeat passes through them again: repeats get darker, thinner or more resonant as they fade.

Each filter can be switched on or off on its own.

**Styles.** Click the arrows on either side of the style name, or scroll over it:

| Style | Character |
|---|---|
| **Smooth** | Gentle, wide resonance |
| **Warm** | Saturated input, linear filter |
| **Monolith** | The Monolith synth's soft-limited, self-oscillating filter |
| **Hot** | Saturating, screaming peak |
| **Acid** | Resonance thins the bass |
| **Raw** | Hard-clipped, buzzy self-oscillation |

**Controls**
- **Type:** **LP**, **BP** or **HP**.
- **Slope:** 12, 24 or 48 dB per octave.
- **FREQ:** 20 Hz to 20 kHz.
- **PEAK:** resonance.
- **PAN:** offsets the cutoff between the left and right lines.

**FILTER ROUTING** (in OUTPUT): **SERIAL** (filter 1, then filter 2) or **PARALLEL** (both, mixed). It only matters when both filters are on.

---

## 7. The LFOs and the envelope

**Each LFO** has:

- **Wave:**
  - **TRI**, **SIN**, **SQR**
  - **RND:** glides between random values
  - **S&H:** jumps between random values
- **BALANCE:** for triangle and sine, it skews the shape from a falling ramp through symmetric (50%) to a rising ramp. For square, it sets the duty cycle.
- **Sync mode:**
  - **FREE:** RATE is in Hz, 0.01 to 50.
  - **STR**, **DOT**, **TRIP:** locked to the host tempo and song position; RATE then picks the note length.
- **RETRIGGER:** restarts the LFO on every MIDI note.

**The envelope** has **ATTACK**, **DECAY**, **SUSTAIN** and **RELEASE**, and a trigger:

- **AUD** (audio): it starts when the input level rises past **THRESH** and releases when it falls below. That makes it an envelope follower.
- **MIDI:** MIDI notes trigger it; THRESH is greyed out.

---

## 8. Modulation

**Drag and drop**
- Drag a **source chip** onto any knob. A slot is created with a sensible starting amount, and the knob gets a coloured ring.
- The chips for LFO 1, LFO 2 and ENV sit on their own panels.
- **VEL** (velocity), **AT** (aftertouch, channel or polyphonic), **KEY** (key tracking), **BEND** (pitch bend) and **WHEEL** (mod wheel) sit on the MODULATION panel.

**Slots.** There are 24. Each one has:
- a **source**; click to pick one
- a **destination**; click to pick one
- an **amount**: turning it left inverts
- a **bypass** switch
- a **delete** button

**Destinations**
- both delay times, pans, feedbacks and cross-feedbacks
- drive
- filter freq, peak and pan
- input gain, mix and output gain
- LFO rate and balance
- every envelope stage
- **every slot's amount**, so one slot can control how much another modulates

**Ranges.** Delay-time modulation spans ±5 octaves:
- Small amounts give tape wow and chorus.
- **Key Track → Time at −100%** makes the delay a comb filter that plays in tune with the keyboard (try the *Key Comb* preset).

![Destination picker](../screenshot-modulation.png)

---

## 9. Output

- **INPUT** (±24 dB): the level into the delay lines. Hotter input drives DRIVE and the saturating filter styles.
- **MIX:** dry/wet balance. On a send, use 100%.
- **OUTPUT** (±24 dB): the output level.
- **CLEAR ECHOES:** silences everything still in the delay lines. MIDI CC 120 (All Sound Off) does the same.

---

## 10. Knobs

- **Drag** up or down; hold **Shift** for fine adjustment. Slot amounts are finer by default.
- **Grab the rim** of a large knob (TIME, FEEDBACK, FREQ, MIX) to turn it in a circle.
- **Mouse wheel** turns a knob.
- **Double-click** to type a value: `375`, `1.5s`, `1.2k`, `L40`, `-6`. **Enter** applies it, **Esc** cancels.
- **Cmd/Ctrl-click** resets to the default.
- The line at the top right of the MODULATION panel shows the name and value of whatever is under the pointer.

---

## 11. A/B, undo and redo

- **A / B** hold two complete settings, modulation included. Click the other letter to switch; the first time, B starts as a copy of A.
- **COPY** copies the current setting into the other slot.
- Every edit is an undo step, including new modulation, tap tempo, preset loads and A/B switches. Strata remembers the last 200.
  - Undo: **UNDO**, or Cmd/Ctrl-Z
  - Redo: **REDO**, or Cmd/Ctrl-Shift-Z or Cmd/Ctrl-Y

---

## 12. MIDI

**MIDI Learn**
1. Click **LEARN**.
2. Click a knob or switch.
3. Move a knob or fader on your controller.
4. Click **LEARN** again (or press Esc) to finish.

- Right-click a control to unassign it.
- **CLEAR ALL CCs** (in the MODULATION panel) removes them all.
- Assignments are saved with your project.

**The MIDI button** switches all MIDI handling off: MIDI sources, triggers, retrigger and learned controllers. Use it in hosts that send MIDI to every effect.

---

## 13. Presets

There are 43 factory presets in six groups, plus your own:

| Group | Presets |
|---|---|
| Echo | Quarter Echo, Dotted Eighth, Slapback, Long Hall, Triplet Taps, Wide Doubler, Clean Digital |
| Ping-Pong | Ping-Pong 1/8, Ping-Pong Dotted, Triplet Bounce, Uneven Pong, Wide Bounce |
| Tape & Dub | Dub Echo, Warm Tape, Worn Cassette, Rolling Tape, Runaway Dub, Lo-Fi Slap, Dub Space |
| Modulation | Chorus, Wide Chorus, Flanger, Jet Flanger, Negative Flanger, Phaser, Vibrato, Rotary |
| Filtered | Sweeping Echo, Darkening, Telephone, Resonant Taps, Acid Echo, Envelope Opens, Ducked Echo |
| FX & Scratch | Scratch, Tape Dive, Stretch Stutter, Haze, Infinite Loop, Wheel Feedback, Key Comb, Bend the Tape |

**Init** is a plain starting point.

**Browsing**
- Click the preset name to open the browser.
- **◀ ▶**, or the mouse wheel over the name, step through factory and user presets.
- A `*` after the name means you have changed the setting since loading it.

**Your own presets**
- **SAVE CURRENT AS…** stores your own presets, in one of these folders:
  - macOS: `~/Library/Audio/Presets/Epsilon Audio/Strata`
  - Windows: `%APPDATA%\Epsilon Audio\Strata\Presets`
  - Linux: `~/.config/epsilon-audio/strata/presets`
- Preset files are plain text and work on every system.
- To delete one, right-click it in the browser and click again to confirm.

---

## 14. Help and hover tips

- Rest the pointer on any control, chip, display or button to see what it does and its current value.
- **HELP** opens the in-plugin pages. The switch at the bottom of the help window turns hover tips on or off.
- **FULL MANUAL (WEB)** opens this manual.

---

## 15. Recipes

- **Dotted-eighth echo:** LEFT DOTTED 1/8, LINK on, FEEDBACK 35%, filter 1 LP around 4 kHz, MIX 25%.
- **Ping-pong:**
  - INPUT PING-PONG
  - both sides STRAIGHT 1/8, FEEDBACK 0
  - CROSS > R and CROSS > L around 60%
- **Dub delay:**
  - TAPE mode, DRIVE 40%
  - FEEDBACK 80-100%, filter 1 HP around 300 Hz, filter 2 LP around 3 kHz
  - ride FEEDBACK above 100% for a moment
- **Chorus:**
  - FREE, TIME about 15 ms, FEEDBACK 0, MIX 50%
  - LFO 1 SIN at about 0.8 Hz onto Left Time and Right Time with small, opposite amounts
- **Flanger:** TIME about 3 ms, FEEDBACK 60%, a slow LFO onto TIME. INVERT for the negative flanger.
- **Scratch:** TAPE mode, SMOOTH short; drag TIME quickly, or modulate it with a fast LFO.
- **Ducked echo:** envelope on AUD with THRESH just under the input peaks; drag ENV onto MIX with a negative amount.

---

## 16. Parameter reference

Every control is automatable. New parameters are only ever added at the end, so sessions and automation keep working across versions.

- **Each side:**
  - Time (1 ms-4 s), Sync (Free, Straight, Dotted, Triplet), Note (1/64-2 bars), Offset (±50%)
  - Pan, Feedback (0-110%), Cross-feedback (0-110%), Invert
- **Link.**
- **Echo:** Mode (Tape, Stretch), Smoothing (1 ms-2 s), Drive, Input (Stereo, Mono, Ping-Pong).
- **Each filter:**
  - On, Style (Smooth, Warm, Monolith, Hot, Acid, Raw)
  - Type (Low Pass, Band Pass, High Pass), Slope (12, 24, 48 dB/oct)
  - Freq (20 Hz-20 kHz), Peak, Pan
- **Filter Routing:** Serial, Parallel.
- **Output:** Input Gain (±24 dB), Mix, Output Gain (±24 dB).
- **Each LFO:**
  - Wave (Triangle, Sine, Square, Random, Sample & Hold), Balance
  - Sync (Free, Straight, Dotted, Triplet), Rate (0.01-50 Hz), Division
  - MIDI Retrigger
- **Envelope:**
  - Attack, Decay, Sustain, Release
  - Threshold (-60 to 0 dB), Trigger (Audio, MIDI)
- **24 modulation slots,** each with Source, Destination, Amount (±100%) and Bypass.
- **MIDI:** on/off.

Strata also reports a few output values that hosts may list (the live delay times, the envelope level and a MIDI Learn message); they are for the interface.

---

## 17. Troubleshooting

- **Strata doesn't appear in my DAW.** Check the install folders above and rescan plugins. On macOS, run the `xattr` command from section 1.
- **The echoes don't follow the tempo.** Choose STRAIGHT, DOTTED or TRIPLET instead of FREE.
- **The feedback runs away.** Lower FEEDBACK or CROSS below 100%, raise DRIVE to tame it, or click CLEAR ECHOES.
- **Changing the time bends the pitch.** That's TAPE mode; choose STRETCH for pitch-free changes.
- **MIDI sources or the MIDI trigger do nothing.** Route MIDI to Strata in your DAW, and check that the MIDI button is on.
- **Hover tips get in the way.** Turn them off with the switch at the bottom of the HELP window.
- **Latency:** Strata has none.
