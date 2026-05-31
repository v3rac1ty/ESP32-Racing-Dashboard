# ESP32 WROOM Racing Dashboard

A **F1 25 PFM21**-inspired sim racing DDU for **ESP32 DevKit / WROOM-style boards** with an **ILI9488 480×320 SPI display**. Data is streamed from **SimHub** over USB serial.

---

## Layout

```
┌──────────────────────────────────────────────────────────────────────┐
│ ●●●●●●●●  RPM DOTS (green → yellow → orange → red)  ●●●●●●●●●●●●  │ 18px
├──────────────────────────────────────────────────────────────────────┤
│     ERS MODE (NONE / MEDIUM / HOT LAP / OVERTAKE)     │ TC │ ABS │P2│ 28px
├──────────────────────────────────────────────────────────────────────┤
│ L3  2/20          │                         │ SOFT ■         WEAR%  │
│ 1:23.456          │                         │ DRS AVAIL             │
│ CURRENT           │                         │ FL          FR        │
│ 1:22.891          │       G E A R           │ ┌────────┐ ┌────────┐ │
│ LAST      -0.565  │        (large)          │ │ 94°    │ │ 96°    │ │
│ 1:22.123          │                         │ │  87%   │ │  85%   │ │
│ BEST              │                         │ └────────┘ └────────┘ │
│━━━━━━━━━━━━━━━━━━━│                         │ RL          RR        │
│ DELTA             │                         │ ┌────────┐ ┌────────┐ │
│   -0.241          │       2 3 4             │ │ 98°    │ │ 97°    │ │
│                   │        KM/H             │ │  91%   │ │  90%   │ │
│ FUEL  8.4L        │                         │ └────────┘ └────────┘ │
│ ---.--- ---.---   │                         │ B  55  │ D  50%       │
│ S1 23.456         │                         │                       │
│ S2 23.789         │                         │                       │
│ S3 24.012         │                         │                       │
├──────────────────────────────────────────────────────────────────────┤
│ ████████████████████████  ERS BAR  ████████████░░░░░░░░░░░  63%    │ 14px
├──────────────────────────────────────────────────────────────────────┤
│ THROTTLE ▁▃▅▇▇▅▃▁▁▃▅▇▇▇▇▅▃▁  /  BRAKE ▁▁▁▃▃▅▅▅▅▃▁▁▁▁▁▁▁▁▁▁       │ 56px
└──────────────────────────────────────────────────────────────────────┘
```

### Center panel overlay (replaces gear when active)

```
┌─────────────────────────┐    ┌─────────────────────────┐
│      SAFETY CAR         │    │      PIT LIMITER         │
│    REQ  +5.000 s        │    │       LIMIT  80 KM/H     │
│       +3.241            │    │          87              │
└─────────────────────────┘    └─────────────────────────┘
       (yellow bg)                     (blue bg)
```

Virtual Safety Car uses the same layout as Safety Car with an orange background.

---

## Panel Guide

### Info Bar (top strip)

| Cell | Content |
|------|---------|
| ERS MODE | Colored badge: NONE (red) / MEDIUM (gold) / HOT LAP (green) / OVERTAKE (lime) |
| T | TC level — yellow when active |
| A | ABS level — blue when active |
| P{n} | Race position (yellow) |

### Left Panel

| Row | Content |
|-----|---------|
| L{n} {pos}/{cars} | Lap counter and race position |
| Current lap time | Green (normal) / Red (invalidated) |
| Last lap time | White + sub-delta right-aligned |
| Best lap time | Magenta |
| Delta | Large — Green (faster) / Red (slower) |
| FUEL | Color by remaining: green → orange → red |
| Gap front / behind | Red (ahead) · Green (behind) |
| S1 / S2 / S3 | Sector times — purple (SB) / green (PB) / yellow / grey |

### Center Panel

| Area | Content |
|------|---------|
| Upper | Current gear — white (1–8), red (R), grey (N) |
| Lower | Speed in KM/H |
| Overlay | Safety Car / Virtual SC / Pit Limiter badge (see above) |

### Right Panel

| Row | Content |
|-----|---------|
| Compound + WEAR% | Colored square + compound name |
| DRS | DRS DISABLED (red) / DRS (grey) / DRS AVAIL (yellow) / DRS ACTIVE (green) |
| FL / FR boxes | Border = tyre temp color; center = wear % |
| RL / RR boxes | Same |
| B {value} | Brake bias |
| D {value}% | Differential on-throttle |

### Bottom Bar

| Strip | Content |
|-------|---------|
| ERS bar | Green >50% · Yellow 25–50% · Red <25% · percentage right-aligned |
| Trace | Scrolling throttle (green) + brake (red) line graph |

---

## DRS States

| Value | Display | Meaning |
|-------|---------|---------|
| 3 | DRS DISABLED (firebrick) | FIA deactivated — SC / VSC / rain |
| 0 | DRS (dark grey) | Not in activation zone |
| 1 | DRS AVAIL (yellow text) | In zone, gap ≤ 1 s |
| 2 | DRS ACTIVE (green bg) | DRS deployed |

---

## Overlays

Overlays replace the center panel content while active.

| Condition | Background | Content |
|-----------|------------|---------|
| Safety Car | Yellow | Title + required delta + live current delta |
| Virtual Safety Car | Orange | Same as SC |
| Pit Limiter | Blue | Title + speed limit + live current speed |

---

## SimHub Protocol

Fields are sent as a semicolon-delimited line prefixed with `SH;`. New fields appended at the end are backward-compatible (they default to safe values if absent):

```
gear; speed; rpmPct; rpmRedline; curLap; lastLap; bestLap; delta;
lap; position; numCars; fuel; lapInvalid; tcLevel; tcActive;
absLevel; absActive; brakeBias; tyrePFL; tyrePFR; tyrePRL; tyrePRR;
tyreTFL; tyreTFR; tyreTRL; tyreTRR; throttle; brake;
diffOnThrottle; diffOffThrottle;
s1Time; s2Time; s3Time; s1Flag; s2Flag; s3Flag;
drsStatus; gapFront; gapBehind; ersPct; ersMode; lastLapDelta;
tyreCompound; tyreWearFL; tyreWearFR; tyreWearRL; tyreWearRR;
safetyCarStatus; scRequiredDelta; pitLimiterActive; pitSpeedLimit
```

---

## Hardware

| Part | Detail |
|------|--------|
| MCU | ESP32 DevKit / WROOM-style board |
| Display | ILI9488 3.5" SPI — 480×320, 18-bit color |

---

## Wiring

```
ILI9488  →  ESP32 WROOM
────────────────────────
VCC      →  3.3V
GND      →  GND
CS       →  GPIO 5
RST      →  GPIO 4
DC       →  GPIO 2
MOSI     →  GPIO 23
SCK      →  GPIO 18
LED/BL   →  GPIO 15  (through 33Ω resistor recommended)
MISO     →  GPIO 19  (optional)
```

> **Note:** If your display has a `T_*` touch header, leave those unconnected.

---

## Software Setup

### 1 — Install PlatformIO

Install the [PlatformIO IDE extension for VS Code](https://platformio.org/install/ide?install=vscode).

### 2 — Build

```bash
pio run -e esp32-wroom
```

### 3 — Flash

```bash
pio run -e esp32-wroom -t upload
```

Hold **BOOT** while the upload starts if it doesn't auto-reset.

---

## SimHub Configuration

1. Open **SimHub** → **Arduino** (or **Custom Serial Device**).
2. Click **"Add serial device"** → select your ESP32's COM port, baud **115200**.
3. Expand the device → **"Edit custom protocol"**.
4. Paste the contents of **`SHCustomProtocol.txt`** into **"Update messages"**.
5. Enable **"Send update messages"**, set rate to **~60 Hz**.
6. Click **Save** and start a session.

---

## Pin Customisation

All pin numbers are in `platformio.ini` as `-D` build flags:

```ini
-DTFT_MOSI=23
-DTFT_SCLK=18
-DTFT_CS=5
-DTFT_DC=2
-DTFT_RST=4
-DTFT_BL=15
```

---

## Color Reference

| Color | Meaning |
|-------|---------|
| Green RPM | Normal RPM |
| Yellow RPM | Approaching redline |
| Orange RPM | Near redline |
| Red RPM | At / above redline |
| Green delta | Faster than session best |
| Red delta | Slower than session best |
| Blue tyre border | Cold (<60 °C) |
| Cyan tyre border | Cool (60–75 °C) |
| Green tyre border | Optimal (75–115 °C) |
| Orange tyre border | Hot (115–130 °C) |
| Red tyre border | Overheating (>130 °C) |
| Green wear % | >75 % remaining |
| Yellow wear % | 50–75 % |
| Orange wear % | 25–50 % |
| Red wear % | <25 % |
| Yellow TC | TC intervention active |
| Blue ABS | ABS intervention active |
| Green fuel | >8 L |
| Orange fuel | 3–8 L |
| Red fuel | <3 L (critical) |

---

## Troubleshooting

**Screen stays white/blank after power-on**
→ Check MOSI/SCK/CS/DC wiring. Swap DC and RST if needed.

**Colors look wrong (washed out / wrong hue)**
→ ILI9488 is 18-bit (RGB666). Ensure `ILI9488_DRIVER=1` is in build flags.

**Garbled display**
→ Lower SPI frequency: change `-DSPI_FREQUENCY=27000000` to `20000000` in `platformio.ini`.

**"Waiting for SimHub..." never clears**
→ Check COM port, baud rate is 115200, and protocol formula is pasted correctly.

**Upload fails**
→ Hold BOOT button while uploading, or press RESET after the upload starts.

**Safety car / pit overlays never appear**
→ Add `safetyCarStatus`, `scRequiredDelta`, `pitLimiterActive`, `pitSpeedLimit` fields to your `SHCustomProtocol.txt` formula at the end of the update string.
