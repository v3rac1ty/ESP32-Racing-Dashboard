#pragma once
#include <TFT_eSPI.h>

// ============================================================
//  Screen dimensions (ILI9488 in landscape = 480 wide, 320 tall)
// ============================================================
static constexpr int SCR_W = 480;
static constexpr int SCR_H = 320;

// ============================================================
//  Colors — Lovely Dashboard palette
// ============================================================
static constexpr uint32_t C_BG         = 0x0000;   // Pure black
static constexpr uint32_t C_WHITE      = 0xFFFF;
static constexpr uint32_t C_YELLOW     = 0xFFE0;
static constexpr uint32_t C_GREEN      = 0x07E0;
static constexpr uint32_t C_RED        = 0xF800;
static constexpr uint32_t C_ORANGE     = 0xFD20;
static constexpr uint32_t C_CYAN       = 0x07FF;
static constexpr uint32_t C_MAGENTA    = 0xF81F;
static constexpr uint32_t C_DARK_GREY  = 0x2104;
static constexpr uint32_t C_MID_GREY   = 0x4208;
static constexpr uint32_t C_LIGHT_GREY = 0x8410;
static constexpr uint32_t C_BLUE       = 0x001F;
static constexpr uint32_t C_TEAL       = 0x0410;
static constexpr uint32_t C_DIM_WHITE  = 0xC618;
static constexpr uint32_t C_PURPLE     = 0xA019;   // ~#A000C8 — F1 session-best purple

// ============================================================
//  Layout regions (all pixel-exact for 480×320)
// ============================================================

// --- RPM dot strip at top ---
static constexpr int RPM_DOT_Y      = 0;
static constexpr int RPM_DOT_H      = 28;   // total height of the dot row
static constexpr int RPM_DOT_COUNT  = 24;   // number of dots across the screen
static constexpr int RPM_DOT_R      = 10;   // dot radius in pixels
// Slot width = SCR_W / RPM_DOT_COUNT = 480/24 = 20px; dot fits with 2px gap each side
// Dot colour zones (dot index, 0-based):
//   0-7   → green   (early RPM)
//   8-14  → yellow
//   15-18 → orange
//   19-23 → red     (redline zone)

// --- Separator line ---
static constexpr int SEP1_Y    = RPM_DOT_H + 2;
static constexpr int SEP1_H    = 1;

// --- Main content area ---
static constexpr int MAIN_Y    = SEP1_Y + SEP1_H + 4;    // ~35
static constexpr int MAIN_H    = 200;

// Left panel (lap times) — 0..155
static constexpr int LP_X      = 0;
static constexpr int LP_W      = 155;

// Vertical divider between left and center
static constexpr int DIV1_X    = LP_X + LP_W;

// Center panel (gear + speed) — 156..323
static constexpr int CP_X      = DIV1_X + 2;
static constexpr int CP_W      = 165;

// Vertical divider between center and right
static constexpr int DIV2_X    = CP_X + CP_W;

// Right panel (delta + position + aids) — 324..479
static constexpr int RP_X      = DIV2_X + 2;
static constexpr int RP_W      = SCR_W - RP_X;

// --- Separator before bottom ---
static constexpr int SEP2_Y    = MAIN_Y + MAIN_H + 3;
static constexpr int SEP2_H    = 1;

// --- Bottom bar (tyre + inputs) ---
static constexpr int BOT_Y     = SEP2_Y + SEP2_H + 3;
static constexpr int BOT_H     = SCR_H - BOT_Y;           // ~74

// Bottom subdivisions
static constexpr int BT_TYRE_X = 0;
static constexpr int BT_TYRE_W = 200;
static constexpr int BT_AID_X  = BT_TYRE_W + 2;
static constexpr int BT_AID_W  = 130;
static constexpr int BT_INP_X  = BT_AID_X + BT_AID_W + 2;
static constexpr int BT_INP_W  = SCR_W - BT_INP_X;

// --- Scrolling trace graph (inside BT_INP section) ---
// Sprite size: GRAPH_W × GRAPH_H × 2 bytes ≈ 19 KB heap
static constexpr int GRAPH_X  = BT_INP_X + 3;
static constexpr int GRAPH_Y  = BOT_Y + 4;
static constexpr int GRAPH_W  = BT_INP_W - 6;   // 140 px — one column per data point
static constexpr int GRAPH_H  = BOT_H - 8;       // 66 px

// ============================================================
//  Sector flag → display color
//  3 = session best (purple)   2 = personal best (green)
//  1 = slower than PB (yellow) 0 = no data (dim)
// ============================================================
inline uint16_t sectorColor(int flag) {
    switch (flag) {
        case 3:  return C_PURPLE;
        case 2:  return C_GREEN;
        case 1:  return C_YELLOW;
        default: return C_MID_GREY;
    }
}
inline uint16_t rpmDotColor(int dotIdx) {
    if (dotIdx >= 19) return C_RED;
    if (dotIdx >= 15) return C_ORANGE;
    if (dotIdx >=  8) return C_YELLOW;
    return C_GREEN;
}
