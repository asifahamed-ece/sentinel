# README Architecture Animations Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Replace the two static ASCII diagrams in the SENTINEL README with three pure-SMIL animated SVG files that show the project structure, the bidirectional transmission path, and the fall-detection state machine.

**Architecture:** Three standalone `.svg` files under the existing `images/` directory, each a self-contained SMIL scene with its own master timeline. No JavaScript, no CSS, no external resources, no build step. `README.md` embeds them at three points and retains the original ASCII art inside `<details>` elements.

**Tech Stack:** SVG 1.1 with SMIL (`<animate>`, `<animateTransform>`, `<animateColor>`, `<set>`, `<clipPath>`). Generic font families only. Verified in Chrome via Playwright.

**Spec:** `specs/2026-09-25-readme-architecture-animations-design.md`

## Global Constraints

These apply to every task. Values are copied verbatim from the spec.

- **SMIL only.** `<animate>`, `<animateTransform>`, `<animateMotion>`, `<animateColor>`, `<set>`, `<mpath>`. No `<script>`, no `<style>` block, no CSS `@keyframes`.
- **No external resources.** No web fonts, no linked images, no external stylesheets. Generic families only: `ui-monospace, monospace` and `sans-serif`.
- **The unanimated markup is the finished diagram.** Every animated element's static attribute values describe the completed, settled state. A renderer that ignores SMIL must show a complete, correct picture.
- **Nothing flashes above 2 Hz.** The emergency pulse runs at 1 Hz.
- **IDs are prefixed per file:** `bn-` for banner, `fm-` for fall machine, `ar-` for architecture.
- **Every file starts with the repo header**, before the `<svg>` element:

  ```xml
  <!-- SPDX-License-Identifier: Apache-2.0 -->
  <!-- Copyright 2026 Team Core Dumped -->
  ```

- **Palette, exact values:**

  | Token | Hex | Use |
  |---|---|---|
  | panel background | `#0d1117` | Baked background rect |
  | panel border | `#21262d` | Card outlines |
  | card fill | `#161b22` | Component cards |
  | primary text | `#e6edf3` | Labels |
  | muted text | `#7d8590` | Captions, units |
  | SAFE | `#2ea043` | Nominal state, normal packets |
  | WARN | `#d29922` | Warning state |
  | DANGER | `#f85149` | High hazard, impact spike |
  | EMERGENCY | `#ff2d2d` | Critical, alarm pulse |
  | link | `#58a6ff` | ESP-NOW and WebSocket channels |
  | accent | `#39c5cf` | Traces, sparklines, active stage |

- **Each file under 100 KB**, target under 20 KB.
- **No committed tooling.** Verification scripts live in `/tmp/opencode/svgcheck/` and are never added to the repository.
- **Do not commit until the user reviews the rendered output.** The user has explicitly reserved the commit decision.

## Deviation from Skill Default: Plan Location

The writing-plans skill defaults to `docs/superpowers/plans/`. In this repository `docs/` is the
LittleFS payload uploaded to the Admin Hub, so plans and specs go in `specs/` instead. This
matches the spec document's own location.

---

## File Structure

| Path | Action | Responsibility |
|---|---|---|
| `images/banner.svg` | Create | Hero banner. 800×180, 6 s loop. Wordmark reveal, ECG trace, status dot, glyph row. |
| `images/fall-machine.svg` | Create | Fall state machine. 960×300, 12 s loop. Accel trace against real thresholds, five stages, latch. |
| `images/architecture.svg` | Create | System architecture and data flow. 960×460, 16 s loop. Three column bands, two channels, return path, 16 s state arc. |
| `README.md` | Modify | Three embeds, two `<details>` wrappers, three alt texts, `images/` in the Project Structure tree. |
| `/tmp/opencode/svgcheck/` | Create, **not committed** | HTTP server, capture harness, pixel measurement. |

Tasks are ordered by dependency. Task 1 proves the two hardest SMIL techniques in isolation so
Tasks 2 and 3 can rely on them. Task 3 depends on both. Task 4 depends on all three.

---

## Task 1: `images/banner.svg`

Proves the clipPath reveal and the dashoffset draw, and establishes the static-state rule.

**Files:**
- Create: `images/banner.svg`
- Create (not committed): `/tmp/opencode/svgcheck/serve.sh`, `/tmp/opencode/svgcheck/measure.py`

**Interfaces:**
- Consumes: the palette and header convention from Global Constraints.
- Produces: the capture workflow used by Tasks 2 and 3, and the reusable pattern "one master loop duration per file, encoded with `keyTimes`/`values` on `dur` equal to the loop".

### Step 1: Build the capture harness

Create `/tmp/opencode/svgcheck/serve.sh`:

```bash
#!/usr/bin/env bash
# Serves the repo so the Playwright MCP browser can load SVGs over http.
# file:// is blocked by the MCP browser, so a server is required.
set -euo pipefail
cd /tmp/opencode/sentinel
exec python3 -m http.server 8777
```

Start it in the background and confirm it responds:

```bash
chmod +x /tmp/opencode/svgcheck/serve.sh
nohup /tmp/opencode/svgcheck/serve.sh >/tmp/opencode/svgcheck/serve.log 2>&1 &
sleep 1.5
curl -s -o /dev/null -w "%{http_code}\n" http://localhost:8777/images/banner.svg
```

Expected: `200`.

Create `/tmp/opencode/svgcheck/measure.py`, which reports the horizontal centre of mass of
pixels of a given colour, so that motion can be proven numerically rather than by eye:

```python
import sys, struct, zlib

def read_png(path):
    d = open(path, 'rb').read(); pos = 8; idat = b''
    while pos < len(d):
        ln = struct.unpack('>I', d[pos:pos+4])[0]
        typ = d[pos+4:pos+8]; data = d[pos+8:pos+8+ln]
        if typ == b'IHDR':
            w, h, bd, ct = struct.unpack('>IIBB', data[:10])
        elif typ == b'IDAT':
            idat += data
        pos += 12 + ln
    raw = zlib.decompress(idat); bpp = 4 if ct == 6 else 3; stride = w * bpp
    out = bytearray(); prev = bytearray(stride); i = 0
    for _y in range(h):
        f = raw[i]; i += 1
        line = bytearray(raw[i:i+stride]); i += stride
        for x in range(stride):
            a = line[x-bpp] if x >= bpp else 0
            b = prev[x]; c = prev[x-bpp] if x >= bpp else 0
            if f == 1: line[x] = (line[x] + a) & 255
            elif f == 2: line[x] = (line[x] + b) & 255
            elif f == 3: line[x] = (line[x] + (a + b) // 2) & 255
            elif f == 4:
                pp = a + b - c
                pa, pb, pc = abs(pp-a), abs(pp-b), abs(pp-c)
                pr = a if (pa <= pb and pa <= pc) else (b if pb <= pc else c)
                line[x] = (line[x] + pr) & 255
        out += line; prev = line
    return w, h, bpp, bytes(out)

w, h, bpp, px = read_png(sys.argv[1])
tr, tg, tb = (int(sys.argv[2][i:i+2], 16) for i in (0, 2, 4))
xs = []
for y in range(h):
    for x in range(w):
        o = y*w*bpp + x*bpp
        r, g, b = px[o], px[o+1], px[o+2]
        if abs(r-tr) < 42 and abs(g-tg) < 42 and abs(b-tb) < 42:
            xs.append(x)
print(f"{sys.argv[1]} px={len(xs)} cx={sum(xs)/len(xs):.1f}" if xs else f"{sys.argv[1]} NONE")
```

### Step 2: Write `images/banner.svg`

`viewBox="0 0 800 180"`, `width="800"`, `height="180"`. Master loop **6 s**.

```xml
<!-- SPDX-License-Identifier: Apache-2.0 -->
<!-- Copyright 2026 Team Core Dumped -->
<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 800 180" width="800" height="180"
     role="img" aria-label="SENTINEL animated banner">
  <defs>
    <linearGradient id="bn-word" x1="0" y1="0" x2="1" y2="0">
      <stop offset="0" stop-color="#39c5cf"/>
      <stop offset="1" stop-color="#58a6ff"/>
    </linearGradient>
    <clipPath id="bn-wipe">
      <rect x="40" y="44" width="720" height="52">
        <animate attributeName="width" values="720;0;720" keyTimes="0;0.2;1"
                 dur="6s" repeatCount="indefinite"/>
      </rect>
    </clipPath>
  </defs>

  <rect width="800" height="180" fill="#0d1117"/>

  <g clip-path="url(#bn-wipe)">
    <text x="40" y="86" font-family="ui-monospace, monospace" font-size="46"
          font-weight="700" letter-spacing="10" fill="url(#bn-word)">SENTINEL</text>
  </g>

  <path d="M40 138 H150 l10 -6 l8 14 l8 -30 l9 22 h60 l10 -5 l8 11 l8 -18 l9 12 H420"
        fill="none" stroke="#39c5cf" stroke-width="2" stroke-linecap="round"
        stroke-linejoin="round" pathLength="100" stroke-dasharray="100" stroke-dashoffset="0">
    <animate attributeName="stroke-dashoffset" values="100;100;0;0;100"
             keyTimes="0;0.1;0.367;0.833;1" dur="6s" repeatCount="indefinite"/>
    <animate attributeName="stroke-opacity" values="1;1;0.35;1;1"
             keyTimes="0;0.4;0.5;0.6;1" dur="6s" repeatCount="indefinite"/>
  </path>

  <circle cx="44" cy="158" r="3.5" fill="#2ea043">
    <animate attributeName="r" values="3.5;3.5;4.5;3.5" keyTimes="0;0.75;0.85;1"
             dur="6s" repeatCount="indefinite"/>
  </circle>

  <text x="58" y="162" font-family="ui-monospace, monospace" font-size="11"
        letter-spacing="3" fill="#7d8590">REAL-TIME WORKER SAFETY MONITOR</text>
</svg>
```

Notes on the two techniques being proven:

- **ClipPath reveal.** The clip rectangle's static `width` is `720`, i.e. fully open, so a
  renderer that ignores SMIL shows the complete wordmark. The animation closes it to `0` and
  reopens it, producing the wipe.
- **Dashoffset draw.** The static `stroke-dashoffset` is `0`, i.e. fully drawn, so the static state
  shows the finished trace. The animation sweeps it to `100` (hidden) and back.

The `stroke-opacity` envelope on the trace thumps it once per 3 s cycle between 2.4 s and 3.6 s,
which is a single pulse well under the 2 Hz ceiling.

### Step 3: Verify the static state

`rsvg-convert` ignores SMIL entirely, so it renders exactly what a SMIL-stripped renderer sees:

```bash
rsvg-convert -w 800 images/banner.svg -o /tmp/opencode/svgcheck/banner_static.png
magick identify /tmp/opencode/svgcheck/banner_static.png
```

Expected: `800x180 PNG`. Open the PNG and confirm the wordmark, the fully drawn ECG trace, the
status dot, and the subtitle are all present. A blank or half-built image means the static-state
rule was violated.

### Step 4: Verify the animation runs

Create `/tmp/opencode/svgcheck/wrap.html` so the SVG is loaded through an `<img>` tag, which is
how GitHub embeds it:

```html
<!DOCTYPE html><meta charset="utf-8">
<body style="margin:0;background:#ffffff">
<img src="/images/banner.svg" width="800">
</body>
```

Serve the repo, navigate the Playwright MCP browser to `http://localhost:8777/../wrap.html`
placed at repo root as `/tmp/opencode/sentinel/wrap.html`, then take two screenshots 0.6 s apart
into `/home/shadow/svgcheck/`.

```bash
python3 /tmp/opencode/svgcheck/measure.py /home/shadow/svgcheck/b1.png 39c5cf
python3 /tmp/opencode/svgcheck/measure.py /home/shadow/svgcheck/b2.png 39c5cf
```

Expected: the two `cx` values differ. The accent colour `#39c5cf` covers both the wordmark and
the trace, so a difference proves the trace dashoffset is animating.

**Do not use `chrome --virtual-time-budget`.** It was tested and does not advance the SMIL clock;
frames render at t=0 regardless of the budget. Real-time waits are required.

### Step 5: Check the size budget

```bash
wc -c images/banner.svg
```

Expected: under 6000 bytes.

---

## Task 2: `images/fall-machine.svg`

Proves the shared `keyTimes` timeline, where one timeline drives both a drawn trace and a
sequence of discrete stage highlights that must not drift apart.

**Files:**
- Create: `images/fall-machine.svg`

**Interfaces:**
- Consumes: the capture harness and `keyTimes` technique from Task 1.
- Produces: the shared-timeline pattern reused by Task 3.

### Step 1: Write `images/fall-machine.svg`

`viewBox="0 0 960 300"`, `width="960"`, `height="300"`. Master loop **12 s**.

Shared timeline, used by the trace reveal and by every stage highlight, so they cannot drift:

```xml
keyTimes="0; 0.167; 0.292; 0.375; 0.667; 0.917; 1"
```

corresponding to 0 s, 2 s, 3.5 s, 4.5 s, 8 s, 11 s, 12 s.

**Geometry.** Plot area x from `60` to `920`, y from `40` (3.8 g) to `190` (0 g). Scale:
`y = 190 - value * 39.5`. Baseline 1.0 g sits at `y = 150.5`.

**Trace points**, matching the real firmware thresholds:

| Segment | Value | y | x range |
|---|---|---|---|
| Baseline | 1.0 | 150.5 | 60 → 380 |
| Free-fall dip | 0.15 | 184.1 | 380 → 430 |
| Impact spike | 3.8 | 39.9 | 430 → 465 |
| Settle | 1.2 | 142.6 | 465 → 640 |
| Flat | 0.9 | 154.5 | 640 → 920 |

**Threshold guides**, dashed `#7d8590` at `stroke-opacity="0.4"`, each with a label:

| Value | y | Label |
|---|---|---|
| 3.0 g | 71.5 | `3.0g IMPACT` |
| 1.5 g | 130.8 | `1.5g RECOVERY FLOOR` |
| 0.3 g | 178.2 | `0.3g FREE-FALL` |

**Stage nodes** along `y = 250`, centred at x `120`, `300`, `480`, `660`, `840`, labelled
`NO_FALL`, `FREE_FALL`, `IMPACT`, `WATCHING`, `CONFIRMED`. Each is a `r=9` circle with
`fill="#0d1117"` and `stroke="#21262d"` statically, animated to `stroke="#39c5cf"` and `r=11`
while active, filling with `#39c5cf` at the peak of its window.

**Latch.** From 8 s a `LATCHED` pill beside `CONFIRMED` is lit and holds until an 11 s `RESET`
pulse, matching the firmware's latching "until BTN_RESET or RESCUE ACK".

**Connector line** joining the five nodes, drawn left to right with a `stroke-dashoffset` reveal
on the same 12 s timeline as the trace.

**Static state**, per the global rule: `stroke-dashoffset="0"` on the trace so it is fully drawn;
all five nodes present and labelled; all three threshold guides visible; `LATCHED` lit.

### Step 2: Verify the static state

```bash
rsvg-convert -w 960 images/fall-machine.svg -o /tmp/opencode/svgcheck/fm_static.png
magick identify /tmp/opencode/svgcheck/fm_static.png
```

Expected: `960x300 PNG` showing the complete trace, all five labelled stages, three threshold
guides, and the latch indicator.

### Step 3: Verify the phase boundaries

Navigate to the wrapper page, then capture at real-time offsets into the 12 s loop and check the
active stage advances. The 12 s loop means captures at roughly 1 s, 3 s, 5 s, 9 s, and 11.5 s
after load show `NO_FALL`, `FREE_FALL`, `WATCHING`, `CONFIRMED` lit, and the reset pulse
respectively.

Because the SMIL clock starts at page load, reload the page before each timed capture. Loading
the wrapper and waiting N seconds is the whole procedure; there is no seek control.

### Step 4: Check the size budget

```bash
wc -c images/fall-machine.svg
```

Expected: under 12000 bytes.

---

## Task 3: `images/architecture.svg`

The main visual. Two stacked packet layers cross-faded on a 16 s master timeline.

**Files:**
- Create: `images/architecture.svg`

**Interfaces:**
- Consumes: the `keyTimes` timeline technique from Task 2 and the `<details>`-free static-state
  rule from Task 1.
- Produces: nothing consumed by later tasks except the filenames and alt text used in Task 4.

### Step 1: Establish the geometry

`viewBox="0 0 960 460"`, `width="960"`, `height="460"`. Master loop **16 s**.

| Element | x | y | w | h |
|---|---|---|---|---|
| Title | 24 | 26 | — | — |
| Column header row | 24 | 70 | — | — |
| Worker card 1 | 24 | 80 | 256 | 160 |
| Worker card 2 | 24 | 252 | 256 | 160 |
| Hub card | 400 | 120 | 160 | 240 |
| Dashboard card | 680 | 96 | 256 | 288 |
| Legend strip | 24 | 430 | — | — |

**Channels**, all straight horizontal runs so packets translate rather than follow a path:

| Channel | From | To | y | Label |
|---|---|---|---|---|
| Node 1 → Hub | 280 | 400 | 160 | `ESP-NOW · 2.4 GHz · CH 1` |
| Node 2 → Hub | 280 | 400 | 332 | `ESP-NOW · 2.4 GHz · CH 1` |
| Hub → Dashboard | 560 | 680 | 240 | `WebSocket /ws · JSON` |
| Dashboard → Hub (return) | 560 | 680 | 268 | `HubCommand · 20B` |

The return channel is the addition that makes the diagram show the system is bidirectional. The
existing ASCII diagram omits it.

### Step 2: Build the packet layers

Two stacked groups per channel, cross-faded on the 16 s timeline.

- **Layer A, nominal cadence.** One `55B` chip per channel, `dur="2s"`,
  `repeatCount="indefinite"`, teal `#39c5cf`. One packet per two seconds is the real firmware
  cadence, so the visual is literally accurate here.
- **Layer B, emergency cadence.** Four `55B` chips per channel staggered by `0.125s`, each
  `dur="0.5s"`, `repeatCount="indefinite"`, red `#ff2d2d`. Four chips per two seconds is one per
  500 ms, the real emergency cadence.

Chips translate with `<animateTransform type="translate">` from `0 0` to `120 0`, which is the
channel length. No `<mpath>` is needed because every channel is a straight horizontal run.

Cross-fade envelope, applied to the outer group of each layer so it also gates the chips'
visibility:

```xml
<!-- Layer A visible 0-10s and 13-16s -->
<animate attributeName="opacity" values="1;1;0;0;1;1"
         keyTimes="0;0.625;0.6875;0.8125;0.9375;1"
         dur="16s" repeatCount="indefinite"/>
<!-- Layer B is the inverse -->
<animate attributeName="opacity" values="0;0;1;1;0;0"
         keyTimes="0;0.625;0.6875;0.8125;0.9375;1"
         dur="16s" repeatCount="indefinite"/>
```

**Honest scale note, to be recorded in a comment in the file:** the ratio between the two
cadences is what communicates the real 2 s to 500 ms firmware change. Absolute on-screen speed is
scaled for legibility.

Radio-wave arcs at the hub's left edge, three staggered per layer with `r` growing 4 → 26 and
`stroke-opacity` falling 0.7 → 0 over `0.5s`, gated by the same layer envelopes.

### Step 3: Build the 16-second state arc

One `keyTimes` timeline drives every state-dependent colour, so the whole scene changes state
together:

```xml
keyTimes="0;0.25;0.25;0.625;0.625;0.8125;0.8125;1"
values="#2ea043;#2ea043;#d29922;#d29922;#ff2d2d;#ff2d2d;#2ea043;#2ea043"
```

corresponding to 0 s, 4 s, 4 s, 10 s, 10 s, 13 s, 13 s, 16 s. Applied to Worker 2's status ring,
the dashboard badge fill, and the packet chips in Layer B.

| Phase | Window | What is shown |
|---|---|---|
| Nominal | 0–4 s | Both nodes SAFE, Layer A teal, 2 s cadence, gauges near empty |
| Warning | 4–8 s | Worker 2 ring and badge turn `#d29922`, packets shift amber, cadence unchanged because the firmware only tightens at DANGER and EMERGENCY |
| Impact | 8–10 s | Worker 2's accel trace dips below 0.3 g then spikes above 3.0 g, trace turns `#f85149` |
| Emergency | 10–13 s | Badge `EMERGENCY`, Layer B red at 0.5 s cadence, banner slides in, alarm ring pulses at 1 Hz |
| Recovery | 13–16 s | `HubCommand · 20B` return chip travels right to left, banner retracts, state latches to SAFE |

The recovery phase is what demonstrates the sticky-SOS latch. It must not be dropped for length.

**Accel traces.** Worker 1 flat with light noise. Worker 2 drawn progressively on the 16 s
timeline: flat to `x=152`, dip to `y≈360` at `x=163`, spike to `y≈308` at `x=176`, then flat,
mapped so that 8 s, 9 s, and 10 s land at x 152, 166, and 180 respectively.

**Emergency banner.** Slides in via a group `translate` from `-240 0` to `0 0` across 9.8 s to
10 s, holds to 12.9 s, retracts by 13 s. Its opacity envelope pulses at exactly 1 Hz, three
pulses across the 3 s window:

```xml
keyTimes="0;0.625;0.65625;0.6875;0.71875;0.75;0.78125;0.8125;1"
values="0;0.92;0.55;0.92;0.55;0.92;0.55;0.92;0"
```

**Dose gauges.** Two rings using the `pathLength="100"` trick already used by the real dashboard
in `docs/index.html`, so `stroke-dashoffset` is expressed as a percentage directly. Both fill
monotonically across the 16 s loop, because shift dose is cumulative in the firmware and does not
reset. Noise ends near 82 %, heat near 70 %.

**Dashboard sparkline.** Jagged polyline with a dashoffset reveal on the 16 s timeline.

### Step 4: Verify the static state

```bash
rsvg-convert -w 960 images/architecture.svg -o /tmp/opencode/svgcheck/ar_static.png
magick identify /tmp/opencode/svgcheck/ar_static.png
```

Expected: `960x460 PNG` showing the complete topology. All four component cards with full labels,
both channel labels legible, one packet resting mid-span on each channel, all rings SAFE green,
banner collapsed. This is the most important single check in the whole plan, because this file is
the fallback for every renderer that strips SMIL.

### Step 5: Verify the four phases

Reload the wrapper page and capture at real-time offsets of roughly 2 s, 6 s, 9 s, and 11.5 s
into the 16 s loop. Confirm:

| Capture | Expected |
|---|---|
| ~2 s | Both rings green, teal packets mid-span, banner absent |
| ~6 s | Worker 2 ring amber, packets amber, banner absent |
| ~9 s | Worker 2 trace spiking, trace red |
| ~11.5 s | Badge `EMERGENCY`, red fast packets, banner visible |
| ~14.5 s | Return chip mid-span travelling left, banner retracting, rings back to green |

### Step 6: Check the size budget

```bash
wc -c images/architecture.svg
```

Expected: under 20000 bytes.

---

## Task 4: README Integration

**Files:**
- Modify: `README.md`

**Interfaces:**
- Consumes: `images/banner.svg`, `images/architecture.svg`, `images/fall-machine.svg` from
  Tasks 1–3.
- Produces: the final deliverable. No task depends on this one.

### Step 1: Insert the banner

After the badge row:

```markdown
[![Platform: ESP32](https://img.shields.io/badge/Platform-ESP32-red.svg)](./)
[![PRs Welcome](https://img.shields.io/badge/PRs-welcome-brightgreen.svg)](CONTRIBUTING.md)

![SENTINEL banner: the wordmark wipes in over a heartbeat trace that pulses once every three seconds, above the words REAL-TIME WORKER SAFETY MONITOR and a green status dot.](images/banner.svg)
```

### Step 2: Replace the architecture ASCII block

Under `## System Architecture`, replace the fenced code block with the embed, keeping the original
ASCII inside `<details>`:

````markdown
![Animated system architecture: two worker wearable nodes on the left send 55-byte telemetry packets over an ESP-NOW 2.4 GHz channel 1 to an admin hub in the middle, which forwards JSON over a WebSocket to the operator dashboard on the right. The dashboard badge steps from SAFE through WARN to EMERGENCY as packet cadence quickens from two seconds to half a second, an emergency banner slides in, and a 20-byte command packet travels back to release the alert.](images/architecture.svg)

<details>
<summary>Plain-text architecture diagram</summary>

```text
...existing ASCII art, unchanged...
```

</details>
````

### Step 3: Replace the fall state machine ASCII block

Under `## Edge Safety Logic & Thresholds`, same treatment:

````markdown
![Animated fall detection state machine: an accelerometer trace draws itself across five stages, dipping below 0.3 g in free fall, spiking above 3.0 g at impact, then settling below 1.5 g. A LATCHED indicator stays lit on the confirmed stage until a reset pulse releases it.](images/fall-machine.svg)

<details>
<summary>Plain-text state machine diagram</summary>

```text
...existing ASCII art, unchanged...
```

</details>
````

### Step 4: Add the SVGs to the `images/` Project Structure tree

In the `## Project Structure` fenced block, add:

```
├── images/                   # Screenshots & animated README visuals
│   ├── ...existing PNG/JPEG entries unchanged...
│   ├── banner.svg            #   Animated hero banner
│   ├── architecture.svg      #   Animated system architecture & live data flow
│   └── fall-machine.svg      #   Animated 4-stage fall detection state machine
```

Place it after `images/` to keep the diagram entries grouped.

### Step 5: Verify the links resolve

```bash
for f in images/banner.svg images/architecture.svg images/fall-machine.svg; do
  printf "%-32s %s\n" "$f" "$(curl -s -o /dev/null -w '%{http_code}' http://localhost:8777/$f)"
done
```

Expected: `200` for all three.

Confirm the relative paths match the directory names exactly, including case, since GitHub paths
are case-sensitive and a mismatch shows a broken image rather than an error.

---

## Task 5: Final Verification Sweep

No new files. Confirms the whole deliverable.

**Files:**
- No changes. This task only verifies.

**Interfaces:**
- Consumes: everything from Tasks 1–4.

### Step 1: Confirm no stray files were added

```bash
git status --short
```

Expected: only the three `images/*.svg` files and `README.md`. Nothing from
`/tmp/opencode/svgcheck/` and no `wrap.html` in the repo.

### Step 2: Confirm the rest of the repository is untouched

```bash
git status --short docs/ node_src/ admin_src/ CONTRIBUTING.md
```

Expected: empty output. The specification places all changes outside the LittleFS payload and the
firmware, so `CONTRIBUTING.md`'s three-file upload list must remain correct as written.

### Step 3: Confirm no forbidden constructs

```bash
rg -n "<script|<style|@keyframes|xlink:href=\"http|@import|font-family=\"[^u]" images/
```

Expected: no matches. Generic font families are the only permitted ones, and
`ui-monospace, monospace` and `sans-serif` both start with `u` or `s`, so the final alternative in
the pattern is what distinguishes them.

### Step 4: Confirm the flash-rate ceiling

Count the emergency pulse cycles in `images/architecture.svg`. The banner envelope steps from
`0.92` to `0.55` and back three times across the 3 s emergency window, which is 1 Hz. Confirm no
animation anywhere has a `dur` short enough to imply more than 2 Hz, that is, no looping
animation with a period under 500 ms.

### Step 5: Idle soak

Leave the wrapper page loaded for three minutes, then capture. The SMIL timeline is absolute
document time rather than rAF-accumulated, so the loop phase must be identical to what a capture
at the same elapsed time would give immediately. A visible jump or reset indicates drift.

### Step 6: Confirm the flash-rate and size budgets one final time

```bash
wc -c images/*.svg
```

Expected: all three under 20000 bytes and under the 100 KB ceiling.

### Step 7: Present for review, then stop

Report the rendered frames and the file sizes to the user. **Do not commit.** The user has
explicitly reserved the commit decision and will review the visual output first. Present the
captured PNGs and the measured sizes, and wait.

---

## Self-Review

**Spec coverage.** Every spec section maps to a task: SMIL technique rules and the static-state
rule to Global Constraints and Tasks 1–3; the two-layer packet cadence to Task 3 Step 2; the
sticky-SOS recovery beat to Task 3 Step 3; threshold values and the latch to Task 2; the banner
elements to Task 1; the three README insertion points and the `<details>` retention to Task 4; the
`<picture>` rejection and the no-GIF decision are honoured by never introducing either; the
accessibility alt text to Task 4; the five verification activities to Task 5; and the risks table
is answered by the static-state rule and the size budget.

**Placeholder scan.** No TBD or TODO. The one genuine risk of vague guidance, the exact
`keyTimes` fractions, is given as literal numbers for the state arc, the layer cross-fade, the
fall machine, and the emergency pulse.

**Type and name consistency.** ID prefixes `bn-`, `fm-`, `ar-` are used consistently. The three
filenames `images/banner.svg`, `images/architecture.svg`, `images/fall-machine.svg` are identical
across the tasks that create them and the task that embeds them. Loop durations 6 s, 12 s, and
16 s match between the task that creates each file and the task that verifies its phases. The
`measure.py` argument order, PNG path then hex colour with no `#`, is consistent with its own
call sites.

**Known non-blocking issue.** Task 1 Step 4 describes the wrapper path awkwardly, because the
wrapper must live inside the served repository root while the harness lives outside it. The
executor should write `/tmp/opencode/sentinel/wrap.html` and navigate to
`http://localhost:8777/wrap.html`. This is a presentation wart in the plan, not an ambiguity in
the work, and is called out here so it is not mistaken for a missing step.
