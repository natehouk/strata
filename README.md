# Strata

**A stereo tape delay plugin for macOS, Windows and Linux (VST3 · AU · CLAP · VST2).**

Part of **[Epsilon Audio](https://github.com/natehouk/epsilon-plugins)**, a family of free, open-source plugins: Monolith, Caldera, Obsidian, Strata, Basalt, Gneiss, Mica, Flint, Granite, Magma, Schist, Slate and Talc.

![Strata](screenshot.png)

Strata is a clean-room design with the feature set of FabFilter Timeless, FabFilter's fourth plugin (October 2006). It was written from scratch, working only from FabFilter's public press release and contemporary reviews. No FabFilter code, binaries, presets, artwork or trademarks were used.

It is the fourth plugin in a family:
- [Monolith](https://github.com/natehouk/monolith), a synth
- [Caldera](https://github.com/natehouk/caldera), a filter effect
- [Obsidian](https://github.com/natehouk/obsidian), a polysynth
- **Strata**, which puts Caldera's filters inside the feedback loop and uses Obsidian's modulation system.

## Download

Get the latest build from [**Releases**](../../releases):

| Platform | File | Contains |
|---|---|---|
| macOS 10.13+ (Apple Silicon + Intel) | `Strata-macos-universal.zip` | VST3, AU, CLAP |
| Windows x64 | `Strata-windows-x64.zip` | VST3, CLAP, VST2 |
| Linux x64 | `Strata-linux-x64.zip` | VST3, CLAP, VST2 |

### Install

| Platform | Copy | To |
|---|---|---|
| macOS | `Strata.vst3` / `Strata.component` / `Strata.clap` | `~/Library/Audio/Plug-Ins/VST3` / `Components` / `CLAP` |
| Windows | `Strata.vst3` (folder) / `Strata.clap` | `C:\Program Files\Common Files\VST3\` / `CLAP\` |
| Linux | `Strata.vst3` / `Strata.clap` | `~/.vst3/` / `~/.clap/` |

**macOS:** the plugins are ad-hoc signed, not notarized. After copying, run:

```
xattr -dr com.apple.quarantine ~/Library/Audio/Plug-Ins/*/Strata*
```

Strata is an audio effect that also accepts MIDI. To use the MIDI sources (velocity, key, wheel and so on) or the MIDI-triggered envelope, route a MIDI track to it. How you do that depends on your DAW.

## Features

**Delay lines (left and right)**
- Each side has its own **time** (1 ms to 4 s), **pan**, **feedback** and **cross-feedback** into the other side.
- **Tempo sync**, per side: **straight, dotted or triplet**, from 1/64 to 2 bars. An **offset** (±50%) pushes or pulls a synced echo off the grid, for swing.
- **Link** makes the right side follow the left side's time settings.
- **Tap tempo**: click in time. The left side takes the tapped time, and the right side keeps its ratio to the left.
- **Invert** flips the polarity of what a side feeds back. With short times, that turns a flanger into a negative (hollow) flanger.
- **Input**:
  - **Stereo**: each input channel feeds its own line.
  - **Mono**: the sum feeds both lines.
  - **Ping-Pong**: the sum enters the left line only, then bounces through cross-feedback.

**Tape and stretch**
- **Tape**: a time change makes the read head speed up or slow down, so the pitch bends the way real tape does. Turn the time knob quickly, or modulate it, to **scratch**. **Smooth** sets how fast the head glides to a new time.
- **Stretch**: a time change crossfades to the new time, so the pitch never changes. Here **Smooth** sets the crossfade length.
- **Drive**: tape-style saturation inside the loop. Each repeat gets a little dirtier, and feedback above 100% stays under control.

**Filters (2, inside the feedback loop)**
- Every repeat passes through the filters again, so repeats get darker, thinner or more resonant as they fade.
- **Six styles:**

  | Style | Character |
  |---|---|
  | **Smooth** | Gentle, wide resonance |
  | **Warm** | Saturated input, linear filter |
  | **Monolith** | The Monolith synth's soft-limited, self-oscillating filter |
  | **Hot** | Saturating, screaming peak |
  | **Acid** | Resonance thins the bass |
  | **Raw** | Hard-clipped, buzzy self-oscillation |

- **Low / band / high pass** at **12, 24 or 48 dB/oct**.
- **Freq**, **Peak** (resonance) and **Pan**: pan offsets the cutoff between the left and right lines.
- **Serial** or **parallel** routing.

**Modulation: 24 slots, drag and drop** (shared with Obsidian)
- **2 LFOs**:
  - Waves: triangle, sine, square, random and sample & hold.
  - **Balance** skews the shape.
  - Free (0.01–50 Hz) or synced to the host: straight, dotted or triplet, locked to song position.
  - **Retrigger** restarts the LFO on each MIDI note.
- **Envelope**: attack, decay, sustain and release. It's triggered either by the **input level** crossing a threshold (an envelope follower) or by **MIDI notes**.
- **MIDI sources**: velocity, aftertouch (channel or polyphonic), key track, pitch bend and mod wheel.
- **Using the slots**:
  - Drag a source chip onto any knob to create a slot. The knob gets a coloured ring showing it's modulated.
  - Each slot has a bipolar **amount** (turn it left to invert), a **bypass** switch and a delete button.
  - Every slot's amount is also a destination, so a slot can control how much another slot modulates.
- Destinations:
  - both delay times, pans, feedbacks and cross-feedbacks
  - drive
  - filter freq, peak and pan
  - input gain, mix and output gain
  - LFO rate and balance
  - every envelope stage
  - every slot's amount
- Delay-time modulation spans ±5 octaves. Small amounts give tape wow and chorus. **Key Track → Time at −100%** makes the delay a comb filter that plays in tune with the keyboard (see the *Key Comb* preset).

![Destination picker](screenshot-modulation.png)

**The echo display** draws every repeat of a single hit on both lines. It includes feedback, cross-feedback, inversion (shown as hollow dots) and how the loop filters affect each pass. It updates live as the times are modulated.

**Workflow** (shared with Caldera and Obsidian)
- **MIDI Learn:**
  1. Click **LEARN**.
  2. Click a control, then move a knob or fader on your controller.
  3. Right-click a control to unassign it, or use **Clear all CCs**.

  Assignments are saved with your project.
- **MIDI** switch: turns all MIDI handling off.
- **Clear Echoes** silences everything still in the delay lines. MIDI CC 120 (All Sound Off) does the same.
- **43 factory presets** in six categories: Echo, Ping-Pong, Tape & Dub, Modulation (chorus, flanger, phaser, vibrato, rotary), Filtered, and FX & Scratch.
- **Save your own presets** as small text files, which load the same on every OS:
  - macOS: `~/Library/Audio/Presets/Epsilon Audio/Strata/`
  - Windows: `%APPDATA%\Epsilon Audio\Strata\Presets\`
  - Linux: `~/.config/epsilon-audio/strata/presets/`
- To delete a user preset, right-click it in the menu, then click it again to confirm.
- **Resizable** window, double-click to **type a value**, **Cmd/Ctrl-click** to reset a control, and hover a control to see its name and value.

### Compared with FabFilter Timeless (2006)

| Timeless | Strata |
|---|---|
| Separate left and right delays, each panned | ✓ |
| Tempo sync with offset, tap tempo, L/R link | ✓ (straight, dotted, triplet) |
| Tape mode: time changes bend pitch (scratching) | ✓ |
| Stretch mode: time changes without pitch change | ✓ |
| Feedback and cross-feedback per channel, phase invert | ✓ |
| 2 filters in the feedback path, 6 characters, LP/BP/HP at 12/24/48 dB, per-filter pan | ✓ |
| 2 LFOs: triangle, square, sine, random, S&H; sync; note sync; balance | ✓ |
| 1 envelope, triggered by audio or MIDI | ✓ |
| 24 drag-and-drop slots, invertible, bypassable, slot levels modulatable | ✓ |
| Sources: velocity, aftertouch, key track, pitch bend, mod wheel | ✓ |
| Delay, echo, chorus, flanger, phaser, scratching | ✓ |
| MIDI Learn, parameter smoothing | ✓ |
| 200+ factory presets, per-component presets | 43, plus saving your own (whole-plugin only) |
| Pro Tools (RTAS/AAX) | Not supported (needs Avid's SDK and signing) |

## Build from source

The plugin is built on [DPF](https://github.com/DISTRHO/DPF):

```
git clone --recursive https://github.com/DISTRHO/DPF.git dpf
make -C plugins/Strata                    # Linux / macOS (macOS also builds the AU)
make -C plugins/Strata WINDOWS=true CC=x86_64-w64-mingw32-gcc CXX=x86_64-w64-mingw32-g++
```

GitHub Actions builds all three platforms on every push. The macOS build is universal and ad-hoc signed. A `v*` tag, or a release published on GitHub, attaches the zips to that release.

## Tests

```
cd tests
g++ -O2 -std=c++11 features.cpp -o features && ./features   # echo timing, sync/offset/link, feedback, invert, ping-pong, tape vs stretch pitch, filters in the loop, LFO/EG/MIDI/slot-on-slot modulation, runaway safety
g++ -O2 -std=c++11 fuzz.cpp -o fuzz && ./fuzz               # random params/MIDI/transport at 44.1–192 kHz: finite, <= 0 dBFS
g++ -O2 -std=c++11 presets.cpp -o presets && ./presets      # plays every factory preset, checks level and tail
g++ -O3 -std=c++11 bench.cpp -o bench && ./bench            # CPU: stretch mode, both filters at 48 dB, 24 slots
```

In the benchmark's worst case (stretch mode crossfading, both filters at 48 dB, all 24 slots active), Strata uses about 2% of one CPU core.

Release checks:
- Steinberg's VST3 validator passes 47/47.
- The interface is exercised with scripted input, including drag-and-drop modulation, tap tempo, the slot pickers, MIDI Learn and presets.

## Source layout

| File | Contents |
|---|---|
| `plugins/Strata/StrataEngine.hpp` | All DSP: delay lines, tape/stretch heads, filters, LFOs, envelope, modulation matrix. Header-only |
| `plugins/Strata/ParamDefs.hpp` | Parameter table, labels, modulation source and destination names, value mapping |
| `plugins/Strata/Presets.hpp` | Factory presets |
| `plugins/Strata/UserPresets.hpp` | User preset files |
| `plugins/Strata/StrataPlugin.cpp` | Plugin wrapper: parameters, programs, MIDI, MIDI Learn, transport |
| `plugins/Strata/StrataUI.cpp` | Custom vector interface (NanoVG) |

New parameters are only ever appended, so saved sessions and automation keep working across versions.

## License

[MIT](LICENSE). Built with [DPF](https://github.com/DISTRHO/DPF) and its bundled NanoVG, which are under the ISC license. `plugins/Strata/ResizeHandle.hpp` comes from DPF's examples and keeps its ISC notice.

"FabFilter" and "FabFilter Timeless" are trademarks of FabFilter Software Instruments. Strata is not affiliated with or endorsed by FabFilter.
