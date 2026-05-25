# ESP32 WROOM Racing Dashboard

A **Lovely Dashboard**-inspired sim racing DDU for the **ESP32 WROOM (30-pin, USB-C)** with an **ILI9488 480×320 SPI display**.

---

## Preview — Layout

```
┌──────────────────────────────────────────────────────────────┐
│████████████████████  RPM BAR (GREEN→ORANGE→RED)  ████████████│  28px
├─────────────────────────────────────────────────────────────-┤
│                       │                  │                    │
│  BEST    0:00.000     │     GEAR         │  DELTA            │
│                       │    (large)       │   +0.000s         │
│  LAST    0:00.000     │                  │                    │
│                       │   SPEED          │   POS  LAP        │
│  CURRENT 0:00.000     │   000 KM/H       │   1/20  1         │
│                       │                  │   FUEL  5.0L      │
├───────────────────────┴──────────────────┴────────────────────┤
│ TYRES           │  TC  ABS  BB   │  THR ████████░░░          │
│ FL 26.0 [88°]   │   2   0  55.0  │  BRK ███░░░░░░░░          │
│ FR 26.2 [91°]   │                │                           │
│ RL 26.8 [96°]   │                │                           │
│ RR 26.5 [93°]   │                │                           │
└─────────────────────────────────────────────────────────────-┘
```

---

## Hardware

| Part | Detail |
|------|--------|
| MCU | ESP32 WROOM-32 (30-pin USB-C devkit) |
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
MISO     →  GPIO 19  (optional, needed for some displays)
```

> **Note:** If your display has a `T_*` touch header, leave those unconnected — touch is not used.

---

## Software Setup

### 1 — Install PlatformIO

Install the [PlatformIO IDE extension for VS Code](https://platformio.org/install/ide?install=vscode).

### 2 — Open and Build

```bash
cd esp32-racing-dash
pio run -e esp32-wroom
```

### 3 — Flash

```bash
pio run -e esp32-wroom -t upload
```

Hold the **BOOT** button on your ESP32 while the upload starts if it doesn't auto-reset.

---

## SimHub Configuration

1. Open **SimHub** and go to **Arduino** (or **Custom Serial Device** if you're on a newer version).
2. Click **"Add serial device"** → select your ESP32's COM port, baud rate **115200**.
3. Expand the device, click **"Edit custom protocol"**.
4. Paste the entire contents of **`SHCustomProtocol.txt`** into the **"Update messages"** field.
5. Enable the slider next to **"Send update messages"**.
6. Set the update rate to **~30 Hz** (or as fast as your USB allows).
7. Click **Save** and start a session — the display activates within seconds.

---

## Pin Customisation

All pin numbers are in `platformio.ini` as `-D` build flags. No header files need editing:

```ini
-DTFT_MOSI=23   ; change to your wiring
-DTFT_SCLK=18
-DTFT_CS=5
-DTFT_DC=2
-DTFT_RST=4
-DTFT_BL=15
```

---

## Display Color Legend

| Color | Meaning |
|-------|---------|
| 🟢 Green RPM bar | Normal RPM |
| 🟡 Yellow RPM bar | Approaching redline (−18%) |
| 🟠 Orange RPM bar | Near redline (−8%) |
| 🔴 Red RPM bar | At/above redline |
| 🟢 Green delta | Faster than best |
| 🔴 Red delta | Slower than best |
| 🔵 Blue tyre temp | Cold (<60°C) |
| 🟢 Green tyre temp | Optimal (75–115°C) |
| 🔴 Red tyre temp | Overheating (>130°C) |
| 🟡 Yellow TC | TC active |
| 🔴 Red fuel | Critical (<5 L) |

---

## Troubleshooting

**Screen stays white/blank after power-on**
→ Check MOSI/SCK/CS/DC wiring. Swap DC and RST if needed.

**Colors look wrong (washed out / wrong hue)**
→ The ILI9488 is 18-bit (RGB666). TFT_eSPI handles this automatically but make sure `ILI9488_DRIVER=1` is in build flags.

**Garbled display**
→ Lower SPI frequency: change `-DSPI_FREQUENCY=27000000` to `20000000` in `platformio.ini`.

**"Waiting for SimHub..." never clears**
→ Check COM port in SimHub matches your ESP32, baud rate is 115200, and Custom Protocol formula is pasted correctly.

**Upload fails**
→ Hold BOOT button while uploading, or press RESET after the upload starts.
