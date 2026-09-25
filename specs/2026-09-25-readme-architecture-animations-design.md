# Animated Architecture Visuals for the SENTINEL README

- **Date:** 2026-09-25
- **Status:** Approved
- **Branch:** `docs/readme-architecture-animations`
- **Author:** Team Core Dumped

## Problem

The SENTINEL README explains the system with two ASCII art blocks: a `System Architecture`
diagram and a fall-detection state machine. Both are accurate but static, and the README is the
first thing a visitor, evaluator, or potential contributor sees. The project's most distinctive
behaviour — the *sticky SOS* latch that holds an emergency until a human acknowledges it — is
invisible in a static diagram, because it is a behaviour over time, not a topology.

This spec adds eye-catching animated visuals to the README that show the project structure,
how it transmits, and how it works.

## Platform constraints

These were verified against GitHub's current markdown sanitizer and the W3C SVG Integration
spec before the approach was chosen. They are not assumptions.

| Technique | Works in README? | Reason |
|---|---|---|
| `<script>` / any JavaScript | No | Sanitizer strips it. Hover and click interactivity are impossible, permanently. |
| Inline `<svg>` pasted into markdown | No | Stripped. The SVG must be a linked file. |
| Linked `.svg` using SMIL (`<animate>`, `<animateTransform>`, `<animateMotion>`, `<set>`, `<mpath>`) | Yes | Img-referenced SVG renders in W3C "secure animated mode": scripting disabled, declarative animation enabled. |
| CSS `@keyframes` inside a linked SVG | Unreliable | Survives inconsistently through GitHub's image proxy. Pure SMIL is the dependable path. |
| `<video>` tag | No | Stripped unless the source is a drag-dropped GitHub asset URL. |
| `<picture>` / `prefers-color-scheme` | Undocumented | Not on the sanitizer's allowed-tag list. Not relied upon. |
| `<details>` / `<summary>` | Yes | On the allowed-tag list. |
| Animated GIF | Yes | Always renders, but multi-megabyte, pixelates on scale, and cannot be reviewed as text in a diff. |

**Decision:** pure-SMIL animated SVG, one theme-agnostic file per visual.

**Consequence accepted:** the visuals animate but are not interactive. There is no hover state, no
click, no tooltips. This is a GitHub platform limit, not a design shortcut.

## Goals

- Replace the two ASCII diagrams with animated equivalents that convey sequence and state over time.
- Show the bidirectional data path: Worker Node → ESP-NOW → Admin Hub → WebSocket → Dashboard, and
  the return path for commands and acknowledgments.
- Make the sticky-SOS latch and the 4-stage fall state machine visible as behaviour.
- Stay consistent with the existing dark industrial dashboard palette in `docs/style.css`.
- Add no runtime dependency, no build step, and no firmware change.

## Non-goals

- No changes to `node_src/`, `admin_src/`, or anything under `docs/`.
- No change to the Hub's LittleFS upload procedure in `CONTRIBUTING.md`; `assets/` is not part of
  the filesystem image.
- No hover, click, or pointer interaction.
- Light/dark theme variants. Each SVG is theme-agnostic by baking in its own panel background.
- A generated GIF fallback. The static-state rule below makes one unnecessary.
- Any new test, build, or tooling committed to the repository.

## Assets

Three files under a new `assets/` directory. Each begins with the repository's standard header:

```xml
<!-- SPDX-License-Identifier: Apache-2.0 -->
<!-- Copyright 2026 Team Core Dumped -->
```

| File | Purpose | Target size |
|---|---|---|
| `assets/banner.svg` | Slim hero beneath the title | ~6 KB |
| `assets/architecture.svg` | System architecture and live data flow | ~18 KB |
| `assets/fall-machine.svg` | 4-stage fall detection state machine | ~12 KB |

README.md is the only existing file modified.

### Palette

Drawn from the existing dashboard so the README and the product read as one system.

| Role | Value | Use |
|---|---|---|
| Panel background | `#0d1117` | Baked into every SVG |
| Panel border | `#21262d` | Card and frame outlines |
| Primary text | `#e6edf3` | Labels and headings |
| Muted text | `#7d8590` | Captions, units, secondary annotation |
| SAFE | `#2ea043` | Nominal state, normal packets |
| WARN | `#d29922` | Warning state |
| DANGER | `#f85149` | High hazard |
| EMERGENCY | `#ff2d2d` | Critical, alarm pulse |
| Link / radio | `#58a6ff` | ESP-NOW and WebSocket channels |
| Accent | `#39c5cf` | Traces, sparklines, active stage highlight |

## SMIL technique rules

These are constraints on how the files may be authored. They exist because violating them silently
breaks rendering on GitHub.

1. **SMIL only.** `<animate>`, `<animateTransform>`, `<animateMotion>`, `<animateColor>`, `<set>`,
   `<mpath>`. No `<script>`, no CSS `<style>` block, no CSS `@keyframes`.
2. **No external resources.** No web fonts, no linked images, no external stylesheets. An
   img-referenced SVG cannot fetch resources at all. Use generic families only:
   `ui-monospace, monospace` and `sans-serif`.
3. **The unanimated markup is the finished diagram.** Every animated element's static attribute
   values describe the completed, settled state. A renderer that ignores SMIL, including social-card
   scrapers, npm's README renderer, and some RSS readers, must show a complete and correct picture
   rather than a half-built or blank one. This is the single most important rule.
4. **Nothing flashes above 2 Hz.** The emergency pulse runs at 1 Hz. This is an accessibility
   requirement, not a taste call.
5. **Unique IDs within each file.** `banner.svg`, `architecture.svg`, and `fall-machine.svg` are
   separate documents, so cross-file collisions are not a concern, but IDs must be unique inside
   each file for `clipPath` and `mpath` references to resolve.
6. **Keep each file under 100 KB** and the node count modest, to stay well inside the image proxy's
   limits.

## Synchronisation technique

All scenes run on a single master loop of 16 seconds (banner 6s, fall machine 12s). Rather than
staggering dozens of independent `begin` offsets, which is verbose and hard to keep correct, each
scene uses one long animation per property with `keyTimes` and `values` encoding the entire cycle.

This is chosen deliberately: SMIL timelines are absolute document time, not rAF-accumulated, so a
`repeatCount="indefinite"` animation cannot drift over a long page session the way a JavaScript
counter would.

Packet cadence changes mid-loop, which a single `animateMotion` cannot express. This is solved with
two stacked packet layers, each internally uniform, cross-faded at the phase boundary:

- **Layer A** — steady cadence, teal, opacity 1 during the SAFE and WARN phases.
- **Layer B** — fast cadence, red, opacity 1 during the EMERGENCY phase.

**Honest note on scale:** the *ratio* between the two cadences is what communicates the real
firmware behaviour of 2 s nominal tightening to 500 ms under emergency. Layer A runs at a
legible 2 s on screen; layer B runs at a legible 0.5 s. The absolute on-screen speed is scaled for
readability and is not a literal millisecond-accurate simulation.

## Asset 1: `assets/banner.svg`

`viewBox="0 0 800 180"`. Loop 6 s.

- **Wordmark.** `SENTINEL` in `ui-monospace, monospace`, `letter-spacing` widened, filled with a
  horizontal `linearGradient` from `#39c5cf` to `#58a6ff`. Revealed by an expanding `clipPath`
  rectangle, 0 s to 1.2 s, then held.
- **ECG trace.** A heartbeat polyline across the lower third, revealed by animating
  `stroke-dashoffset` from full length to 0 over 0.6 s to 2.2 s, then held.
- **Heartbeat repeat.** After the reveal, the trace's `stroke-opacity` pulses once per 3 s cycle,
  a single 1 Hz-style thump rather than a continuous shimmer.
- **Status dot.** A `#2ea043` circle to the left of the subtitle, radius pulsing 3 to 4.5 px once
  per 6 s cycle.
- **Subtitle.** `REAL-TIME WORKER SAFETY MONITOR`, `#7d8590`, `letter-spacing` 3, opacity fading
  0.4 s to 1.6 s.
- **Glyph row.** Five small 14 px marks along the right edge, one each for MPU6050, DHT22, MQ-2,
  MAX30102, and ESP-NOW, fading in staggered 0.15 s apart from 1.0 s.

**Static state:** wordmark fully visible and unrevealed-clip, trace fully drawn, subtitle at full
opacity, dot visible, glyphs present.

## Asset 2: `assets/architecture.svg`

`viewBox="0 0 960 440"`. Loop 16 s. Three column bands.

```text
 WORKER WEARABLE 1 / 2   --ESP-NOW 2.4GHz CH1-->   ADMIN HUB   --WebSocket /ws-->   OPERATOR DASHBOARD
 MPU6050 DHT22 MQ-2 MAX30102                        ESP-NOW Peer Manager         status badge
 OLED + status LED + buzzer                         SoftAP "SENTINEL-HUB"        telemetry sparkline
 live accelerometer trace                           AsyncWebServer              noise / heat dose gauges
                                                   LittleFS                    emergency banner
```

**Left column.** Two worker cards stacked, labelled `WORKER 1` and `WORKER 2`, each with a zone
caption (`CNC Bay`, `Welding Cell`). Each card shows a status ring, a miniature accelerometer
trace, and a battery bar. The ring colour is driven by the state machine in the loop below.

**Node to hub link.** A horizontal channel labelled `ESP-NOW · 2.4 GHz · CH 1` in `#58a6ff`. Two
stacked packet layers travel left to right along it. Each packet is a 22×12 rounded rect in
`#39c5cf` labelled `55B`, matching the real `SentinelPacket` size. The hub emits three expanding
radio-wave arcs on each packet arrival, stroke opacity fading 0.7 to 0 over 0.5 s.

**Hub.** A card labelled `ADMIN HUB` listing its four responsibilities as `ESP-NOW Peer Manager`,
`SoftAP "SENTINEL-HUB"`, `AsyncWebServer + WebSockets`, and `LittleFS`. A return arrow beneath the
outbound link, labelled `HubCommand · 20B`, travelling right to left to show the command and
acknowledgment path. This return path is what makes the system bidirectional, and it is currently
absent from the README's ASCII diagram.

**Hub to dashboard link.** A horizontal channel labelled `WebSocket /ws · JSON` in `#58a6ff`,
carrying JSON packet chips coloured by the source node's current safety state.

**Right column.** A miniature of the real dashboard from `docs/index.html`: a status badge, a
telemetry sparkline with an animated `stroke-dashoffset` reveal, two circular dose gauges whose
`stroke-dashoffset` fills, and the emergency banner in its collapsed position.

**The 16-second loop.**

| Phase | Window | What is shown |
|---|---|---|
| Nominal | 0–4 s | Both nodes SAFE. Layer A packets, teal, 2 s cadence. Gauges near zero. |
| Warning | 4–8 s | Worker 2 ring turns `#d29922`, badge reads `WARN`. Layer A packets shift from teal to amber. Worker 1 stays SAFE. Cadence is unchanged, because the firmware only tightens the beacon at `DANGER` and `EMERGENCY`. |
| Impact | 8–10 s | Worker 2's accel trace dips then spikes. Trace turns `#f85149`. |
| Emergency | 10–13 s | Badge reads `EMERGENCY`. Layer B packets, red, 0.5 s cadence. Emergency banner slides in. Alarm ring pulses at 1 Hz. |
| Recovery | 13–16 s | Operator ACKs; the `HubCommand · 20B` return packet travels left; banner retracts; state latches back to SAFE. |

The recovery phase exists to show the sticky-SOS latch releasing. Without it the animation would
misrepresent the system's most distinctive behaviour.

**Static state:** the complete topology. All four component cards present with full labels, both
channel labels legible, one packet resting mid-span on each link, all rings in the SAFE green, the
emergency banner collapsed. A reader on a renderer that strips SMIL sees the entire architecture.

## Asset 3: `assets/fall-machine.svg`

`viewBox="0 0 960 300"`. Loop 12 s.

An accel trace draws itself across the upper half, with five stage nodes along the lower half.

**Trace shape**, in g, matching the firmware's real thresholds:

| Segment | Value | Stage |
|---|---|---|
| Baseline | 1.0 | `NO_FALL` |
| Dip | 0.15 | `FREE_FALL_DETECTED`, below the 0.3 g threshold |
| Spike | 3.8 | `IMPACT_DETECTED`, above the 3.0 g threshold |
| Settle | 1.2 | `WATCHING_RECOVERY`, sustained past 1000 ms |
| Flat | 0.9 | `FALL_CONFIRMED`, below 1.5 g past 3000 ms |

**Threshold guides.** Horizontal dashed lines at 0.3 g, 1.5 g, and 3.0 g in `#7d8590` at 40%
opacity, each labelled with its value and the condition it encodes.

**Stage nodes.** `NO_FALL`, `FREE_FALL`, `IMPACT`, `WATCHING`, `CONFIRMED`, left to right. The
active node fills with `#39c5cf` and scales to 1.15 as the trace reaches it, then returns to the
unfilled state. A connector line joins consecutive nodes, drawing in left to right as the trace
advances.

**Latching.** From 8 s, a `LATCHED` indicator beside `CONFIRMED` stays lit and does not advance
until a `RESET` pulse at 11 s, matching the firmware's `FALL_CONFIRMED` latching "until BTN_RESET
or RESCUE ACK". The trace stops advancing while latched, which is the visual point.

**Timing.** The trace's `stroke-dashoffset` reveal and the stage highlights share one 12 s
`keyTimes` timeline so they cannot drift apart:

```xml
keyTimes="0; 0.167; 0.292; 0.375; 0.667; 0.917; 1"
```

corresponding to 0 s, 2 s, 3.5 s, 4.5 s, 8 s, 11 s, 12 s.

**Static state:** the full trace drawn end to end, all five stage nodes labelled and filled, all
three threshold guides visible, `LATCHED` lit.

## README integration

Three insertion points, and no others.

1. **Banner.** After the badge row, before the `---` and the Table of Contents. This is the first
   thing seen after the title and badges without pushing the TOC off screen.
2. **Architecture.** Under `## System Architecture`, replacing the ASCII block. The ASCII block is
   retained inside a `<details>` element so it stays available to screen readers, text search, and
   copy-paste:

   ```markdown
   <details>
   <summary>Plain-text architecture diagram</summary>

   ```text
   ...existing ASCII art, unchanged...
   ```
   </details>
   ```

3. **Fall machine.** Under `## Edge Safety Logic & Thresholds`, replacing the ASCII state machine
   block, with the same `<details>` treatment.

Markdown form for each embed:

```markdown
![Full-sentence description of what the animation shows and how it moves.](assets/<name>.svg)
```

The alt text is the accessible name for an `<img>`-embedded SVG, so it is a complete sentence
describing both content and motion, not a label like "architecture diagram".

Additionally, `assets/` is added to the `## Project Structure` tree in the README so the new
directory is documented alongside the existing ones.

## Accessibility

- Alt text on every embed, as described above.
- The original ASCII diagrams are preserved in `<details>`, giving a non-visual, text-based
  equivalent of the same information.
- No flashing above 2 Hz. The emergency pulse is 1 Hz.
- Text contrast against the `#0d1117` panel background meets WCAG AA at the sizes used.

**Known limitation, stated plainly:** `prefers-reduced-motion` cannot suppress SMIL. CSS media
queries have no hook into SMIL animation, and the media query cannot be read from inside an
img-referenced SVG. The mitigations are slow, subtle motion and a static state that is already a
complete, correct diagram. This limitation is documented here rather than papered over, and it is
the reason the motion is deliberately restrained.

## Verification

The repository has no test infrastructure and this change adds none. Verification is performed
locally and the tooling is not committed.

1. **Phase capture.** Render each SVG in headless Chrome with `--virtual-time-budget` set to
   specific timestamps to advance the SMIL clock to an exact instant, capturing a frame at each
   loop phase. This confirms every phase renders and that the phases agree with the tables above.
2. **Static-state check.** Render with SMIL disabled to confirm the finished-state rule holds and
   no renderer sees a blank or half-built diagram.
3. **Light and dark.** View on github.com in both themes and confirm the baked panel background
   reads correctly in each.
4. **Narrow viewport.** Confirm legibility at mobile width, since README content is centred and
   scaled down on small screens.
5. **Idle soak.** Leave a rendered SVG running for several minutes and confirm the loop does not
   drift, which the absolute-timeline technique is chosen to guarantee.

## Risks

| Risk | Mitigation |
|---|---|
| GitHub changes its sanitizer and strips SMIL | Static-state rule means the README degrades to a correct static diagram, not a broken one. |
| Image proxy rejects the file for size | Budget of 100 KB per file against a ~20 KB target. |
| Animations read as "flashy" rather than informative | Motion is restrained and every movement encodes a real firmware behaviour or threshold. |
| SVGs drift out of sync with the firmware they describe | Threshold values and state names are taken directly from `node_src/packet_defs.h` and the README's own tables, and must be re-checked if those change. |
| The visuals become stale and nobody notices | Accepted. A CI check that re-renders frames would exceed this change's scope. |

## Out of scope

Any modification to `node_src/`, `admin_src/`, or `docs/`. Any change to the LittleFS upload
procedure. Any GIF rendering. Any light and dark variant. Any new build step, dependency, or
committed tooling.
