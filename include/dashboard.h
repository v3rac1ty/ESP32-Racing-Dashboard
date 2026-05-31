#pragma once
#include <TFT_eSPI.h>

// ============================================================
//  ESP32 ILI9488 480×320 — F1 25 PFM21 Dashboard
//  Exact color/layout replica of WYS1.djson (2340×1314 canvas)
//  scaled to 480×320 landscape.
// ============================================================

static constexpr int SCR_W = 480;
static constexpr int SCR_H = 320;

// ============================================================
//  F1 25 PFM21 Color Palette
//  Colors sourced directly from WYS1.djson bindings
// ============================================================
static constexpr uint16_t C_BG           = 0x0000; // #000000
static constexpr uint16_t C_WHITE        = 0xFFFF; // #FFFFFF
static constexpr uint16_t C_GREEN        = 0x07E0; // #00FF00  current lap / limgreen ERS
static constexpr uint16_t C_MAGENTA      = 0xF81F; // #FF00FF  best lap
static constexpr uint16_t C_YELLOW       = 0xFFE0; // #FFFF00  race position
static constexpr uint16_t C_RED          = 0xF800; // #FF0000  delta+, tyre hot
static constexpr uint16_t C_ORANGE       = 0xFDA0; // ~#FFA500 gap-to-leader / ERS pct
static constexpr uint16_t C_CYAN         = 0x07FF; // #00FFFF  tyre cold
static constexpr uint16_t C_DARK_GREY    = 0x2104;
static constexpr uint16_t C_MID_GREY     = 0x4208;
static constexpr uint16_t C_LIGHT_GREY   = 0x8410;
// F1 25 badge colors
static constexpr uint16_t C_DODGER_BLUE  = 0x1CBF; // #1E90FF  BB badge
static constexpr uint16_t C_TEAL_BADGE   = 0x4699; // #48D1CC  Diff badge
// ERS deploy-mode colors (from WYS1 Bindings)
static constexpr uint16_t C_ERS_NONE     = 0xB104; // Firebrick  #B22222
static constexpr uint16_t C_ERS_MEDIUM   = 0xDD24; // Goldenrod  #DAA520
static constexpr uint16_t C_ERS_HOTLAP   = 0x2444; // ForestGreen #228B22
static constexpr uint16_t C_ERS_OVERTAKE = 0x3666; // LimeGreen  #32CD32
// Sector-time colors
static constexpr uint16_t C_PURPLE       = 0xA019; // #A000C8  session best
// Tyre compound badge colors
static constexpr uint16_t C_SOFT_RED     = 0xF800; // red   — SOFT
static constexpr uint16_t C_MED_YELLOW   = 0xFFE0; // yellow — MED
static constexpr uint16_t C_HARD_WHITE   = 0xFFFF; // white  — HARD
static constexpr uint16_t C_INT_GREEN    = 0x07E0; // green  — INT
static constexpr uint16_t C_WET_BLUE     = 0x001F; // blue   — WET

// ============================================================
//  Layout Constants
// ============================================================

// ── RPM LED strip (top, full width) ──────────────────────────
static constexpr int RPM_Y    = 0;
static constexpr int RPM_H    = 18;
static constexpr int RPM_DOTS = 24;
static constexpr int RPM_R    = 7;   // dot radius px

// ── Separator ────────────────────────────────────────────────
static constexpr int SEP1_Y = RPM_H;   // 18

// ── Info bar ─────────────────────────────────────────────────
// [ERS_MODE | TC | ABS | P pos]
static constexpr int IB_Y   = SEP1_Y + 1;  // 19
static constexpr int IB_H   = 28;           // 28 px tall

static constexpr int IB_ERS_X = 0,   IB_ERS_W = 320; // ERS mode badge
static constexpr int IB_TC_X  = 320, IB_TC_W  = 48;  // TC level
static constexpr int IB_ABS_X = 368, IB_ABS_W = 48;  // ABS level
static constexpr int IB_POS_X = 416, IB_POS_W = 64;  // P{pos}

// ── Separator ────────────────────────────────────────────────
static constexpr int SEP2_Y = IB_Y + IB_H; // 47

// ── Main body ────────────────────────────────────────────────
static constexpr int MAIN_Y = SEP2_Y + 1; // 48
static constexpr int MAIN_H = 200;        // 48-247

// Three vertical panels
static constexpr int LP_X = 0,   LP_W = 130; // left: lap times / info
static constexpr int CP_X = 130, CP_W = 200; // center: gear (dominant)
static constexpr int RP_X = 330, RP_W = 150; // right: tyres / aids

// ── Separator ────────────────────────────────────────────────
static constexpr int SEP3_Y = MAIN_Y + MAIN_H; // 248

// ── Bottom bar ───────────────────────────────────────────────
static constexpr int BOT_Y  = SEP3_Y + 1; // 249
static constexpr int BOT_H  = SCR_H - BOT_Y; // 71 px

// ERS bar row (top of bottom bar)
static constexpr int BOT_ERS_Y  = BOT_Y; // 249
static constexpr int BOT_ERS_H  = 14;

// Trace
static constexpr int TRACE_X    = 0;
static constexpr int TRACE_Y    = BOT_ERS_Y + BOT_ERS_H + 1; // 264
static constexpr int TRACE_W    = 480;
static constexpr int TRACE_H    = SCR_H - TRACE_Y; // 56 px

// Left panel stacked sector rows
static constexpr int LP_SEC_Y       = MAIN_Y + 146;
static constexpr int LP_SEC_ROW_H   = 12;

// ============================================================
//  Helper: sector color
//  3=purple(SB)  2=green(PB)  1=yellow(slower)  0=no data
// ============================================================
inline uint16_t sectorColor(int flag) {
    switch (flag) {
        case 3:  return C_PURPLE;
        case 2:  return C_GREEN;
        case 1:  return C_YELLOW;
        default: return C_MID_GREY;
    }
}

// RPM dot color by index (24 dots: 0-7 green, 8-14 yellow, 15-18 orange, 19-23 red)
inline uint16_t rpmDotColor(int idx) {
    if (idx >= 19) return C_RED;
    if (idx >= 15) return C_ORANGE;
    if (idx >=  8) return C_YELLOW;
    return C_GREEN;
}

// ERS mode → badge background color and label
inline uint16_t ersModeColor(int mode) {
    switch (mode) {
        case 1: return C_ERS_MEDIUM;
        case 2: return C_ERS_HOTLAP;
        case 3: return C_ERS_OVERTAKE;
        default: return C_ERS_NONE;
    }
}
inline const char* ersModeLabel(int mode) {
    switch (mode) {
        case 1: return "MEDIUM";
        case 2: return "HOT LAP";
        case 3: return "OVERTAKE";
        default: return "NONE";
    }
}

// Tyre compound → color + letter
inline uint16_t compoundColor(const String& c) {
    if (c == "SOFT")  return C_SOFT_RED;
    if (c == "MED")   return C_MED_YELLOW;
    if (c == "HARD")  return C_HARD_WHITE;
    if (c == "INT")   return C_INT_GREEN;
    if (c == "WET")   return C_WET_BLUE;
    return C_LIGHT_GREY;
}
inline char compoundLetter(const String& c) {
    if (c == "SOFT") return 'S';
    if (c == "MED")  return 'M';
    if (c == "HARD") return 'H';
    if (c == "INT")  return 'I';
    if (c == "WET")  return 'W';
    return '?';
}

// Tyre temperature → color (blue cold → green optimal → red hot)
inline uint16_t tyreTempColor(float t) {
    if (t < 60)  return 0x001F; // cold blue
    if (t < 75)  return C_CYAN;
    if (t < 115) return C_GREEN;
    if (t < 130) return C_ORANGE;
    return C_RED;
}
