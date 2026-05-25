#pragma once
#include <TFT_eSPI.h>
#include "dashboard.h"
#include "simhub_protocol.h"

// ============================================================
//  DashRenderer — draws the Lovely-style dashboard on ILI9488
//  Principle: only redraw regions that have changed.
// ============================================================
class DashRenderer {
public:
    DashRenderer(TFT_eSPI& tft) : _tft(tft) {}

    // Call once in setup()
    void begin() {
        _tft.begin();
        _tft.setRotation(1);           // Landscape, USB connector on left
        _tft.fillScreen(C_BG);
        _drawStaticChrome();           // Labels, dividers, boxes that never change
        _drawIdleScreen();             // Show "Waiting for SimHub..." until data arrives
    }

    // Call every loop() iteration.
    // Only repaints regions where values changed.
    void update(const SimHubData& d) {
        if (!d.valid) return;

        _updateRpmDots(d.rpmPct, d.rpmRedline);
        _updateGear(d.gear);
        _updateSpeed(d.speed);
        _updateLapTimes(d.curLap, d.lastLap, d.bestLap, d.lapInvalid);
        _updateDelta(d.delta);
        _updateSessionInfo(d.lap, d.position, d.numCars, d.fuel);
        _updateAids(d.tcLevel, d.tcActive, d.absLevel, d.absActive, d.brakeBias);
        _updateTyres(d.tyrePFL, d.tyrePFR, d.tyrePRL, d.tyrePRR,
                     d.tyreTFL, d.tyreTFR, d.tyreTRL, d.tyreTRR);
        _updateInputBars(d.throttle, d.brake);
    }

private:
    TFT_eSPI& _tft;

    // ---- Cached previous values ----
    int    _prevRpmPct    = -1;
    int    _prevRpmRed    = -1;
    String _prevGear;
    int    _prevSpeed     = -1;
    String _prevCurLap;
    String _prevLastLap;
    String _prevBestLap;
    bool   _prevLapInv    = false;
    String _prevDelta;
    int    _prevLap       = -1;
    int    _prevPos       = -1;
    int    _prevCars      = -1;
    float  _prevFuel      = -1;
    int    _prevTC        = -1;
    bool   _prevTCA       = false;
    int    _prevABS       = -1;
    bool   _prevABSA      = false;
    float  _prevBB        = -1;
    float  _prevTyreP[4]  = {-1,-1,-1,-1};
    float  _prevTyreT[4]  = {-1,-1,-1,-1};
    int    _prevThr       = -1;
    int    _prevBrk       = -1;
    bool   _idleCleared   = false;

    // ============================================================
    //  Static chrome — drawn once
    // ============================================================
    void _drawStaticChrome() {
        // Draw unlit RPM dot outlines
        static constexpr int SLOT = SCR_W / RPM_DOT_COUNT;
        static constexpr int CY   = RPM_DOT_Y + RPM_DOT_H / 2;
        for (int i = 0; i < RPM_DOT_COUNT; i++) {
            int cx = SLOT / 2 + i * SLOT;
            _tft.drawCircle(cx, CY, RPM_DOT_R, C_DARK_GREY);
        }

        // Separator lines
        _tft.drawFastHLine(0, SEP1_Y, SCR_W, C_MID_GREY);
        _tft.drawFastHLine(0, SEP2_Y, SCR_W, C_MID_GREY);

        // Vertical dividers in main area
        _tft.drawFastVLine(DIV1_X, MAIN_Y, MAIN_H, C_MID_GREY);
        _tft.drawFastVLine(DIV2_X, MAIN_Y, MAIN_H, C_MID_GREY);

        // Bottom area dividers
        _tft.drawFastVLine(BT_TYRE_W,       BOT_Y, BOT_H, C_MID_GREY);
        _tft.drawFastVLine(BT_AID_X + BT_AID_W, BOT_Y, BOT_H, C_MID_GREY);

        // ---- Left panel labels ----
        _tft.setTextColor(C_LIGHT_GREY, C_BG);
        _tft.setTextSize(1);
        _tft.setFreeFont(nullptr);  // default GLCD font

        _tft.setTextSize(1);
        _labelText("BEST",    LP_X + 4,  MAIN_Y + 6,       C_CYAN);
        _labelText("LAST",    LP_X + 4,  MAIN_Y + 72,      C_DIM_WHITE);
        _labelText("CURRENT", LP_X + 4,  MAIN_Y + 138,     C_WHITE);

        // ---- Right panel labels ----
        _labelText("DELTA",   RP_X + 4,  MAIN_Y + 6,       C_LIGHT_GREY);
        _labelText("POS",     RP_X + 4,  MAIN_Y + 110,     C_LIGHT_GREY);
        _labelText("LAP",     RP_X + 80, MAIN_Y + 110,     C_LIGHT_GREY);
        _labelText("FUEL",    RP_X + 4,  MAIN_Y + 150,     C_LIGHT_GREY);

        // ---- Bottom tyre labels ----
        _labelText("TYRES",   BT_TYRE_X + 4,  BOT_Y + 2,  C_LIGHT_GREY);
        _labelText("FL",      BT_TYRE_X + 4,  BOT_Y + 16, C_CYAN);
        _labelText("FR",      BT_TYRE_X + 102,BOT_Y + 16, C_CYAN);
        _labelText("RL",      BT_TYRE_X + 4,  BOT_Y + 46, C_CYAN);
        _labelText("RR",      BT_TYRE_X + 102,BOT_Y + 46, C_CYAN);

        // ---- Bottom aids labels ----
        _labelText("TC",      BT_AID_X + 4,   BOT_Y + 16, C_YELLOW);
        _labelText("ABS",     BT_AID_X + 46,  BOT_Y + 16, C_BLUE);
        _labelText("BB",      BT_AID_X + 94,  BOT_Y + 16, C_MAGENTA);

        // ---- Input bar labels ----
        _labelText("THR",     BT_INP_X + 4,   BOT_Y + 2,  C_GREEN);
        _labelText("BRK",     BT_INP_X + 4,   BOT_Y + 38, C_RED);

        // ---- Center panel "GEAR" and "KM/H" sublabels ----
        _labelText("GEAR",    CP_X + (CP_W/2) - 16, MAIN_Y + 4, C_DARK_GREY);
        _labelText("KM/H",    CP_X + (CP_W/2) - 14, MAIN_Y + 153, C_MID_GREY);
    }

    void _drawIdleScreen() {
        _tft.setTextColor(C_MID_GREY, C_BG);
        _tft.setTextSize(2);
        _tft.setCursor(CP_X + 8, MAIN_Y + 80);
        _tft.print("Waiting for");
        _tft.setCursor(CP_X + 8, MAIN_Y + 105);
        _tft.print("  SimHub...");
        _tft.setTextSize(1);
    }

    void _clearIdle() {
        if (!_idleCleared) {
            _tft.fillRect(CP_X + 1, MAIN_Y + 78, CP_W - 2, 50, C_BG);
            _idleCleared = true;
        }
    }

    // ============================================================
    //  RPM Dots — 24 circles across the top, light up progressively
    //  Green (0-7) → Yellow (8-14) → Orange (15-18) → Red (19-23)
    //  The redline parameter maps to which dot the redline sits on,
    //  so dots at/beyond redline always show red regardless of zone.
    // ============================================================
    void _updateRpmDots(int pct, int redline) {
        if (pct == _prevRpmPct && redline == _prevRpmRed) return;

        pct     = constrain(pct, 0, 100);
        redline = constrain(redline, 1, 100);

        // How many dots are currently lit
        // Scale: dot i lights when pct >= (i+1) * (100/RPM_DOT_COUNT)
        // We map pct linearly to 0..RPM_DOT_COUNT
        int litNow  = (pct  * RPM_DOT_COUNT + 50) / 100;  // rounded
        int litPrev = (_prevRpmPct < 0) ? -1 :
                      (_prevRpmPct * RPM_DOT_COUNT + 50) / 100;

        // Redline: which dot index is the first "red" dot (override colour)
        int redlineDot = (redline * RPM_DOT_COUNT + 50) / 100;

        static constexpr int SLOT  = SCR_W / RPM_DOT_COUNT;  // 20px
        static constexpr int CY    = RPM_DOT_Y + RPM_DOT_H / 2;

        // Only repaint dots that changed state
        for (int i = 0; i < RPM_DOT_COUNT; i++) {
            bool wasLit = (i < litPrev);
            bool isLit  = (i < litNow);
            bool redlineChanged = (redline != _prevRpmRed);

            if (wasLit == isLit && !redlineChanged) continue;

            int cx = SLOT / 2 + i * SLOT;

            if (isLit) {
                // Colour override: dots at or beyond the redline threshold go red
                uint16_t col = (i >= redlineDot) ? C_RED : rpmDotColor(i);
                _tft.fillCircle(cx, CY, RPM_DOT_R, col);
                // Bright inner highlight — tiny white dot for "LED" look
                _tft.fillCircle(cx - 3, CY - 3, 2, 0xFFFF);
            } else {
                // Unlit: dark filled circle with a dim outline ring
                _tft.fillCircle(cx, CY, RPM_DOT_R, C_BG);
                _tft.drawCircle(cx, CY, RPM_DOT_R, C_DARK_GREY);
            }
        }

        _prevRpmPct = pct;
        _prevRpmRed = redline;
    }

    // ============================================================
    //  Gear — large number, center panel
    // ============================================================
    void _updateGear(const String& gear) {
        if (gear == _prevGear) return;
        _clearIdle();

        // Clear previous gear
        _tft.fillRect(CP_X + 1, MAIN_Y + 18, CP_W - 2, 125, C_BG);

        uint16_t col = C_YELLOW;
        if (gear == "R") col = C_RED;
        else if (gear == "N") col = C_LIGHT_GREY;

        _tft.setTextColor(col, C_BG);
        _tft.setTextDatum(MC_DATUM);
        _tft.setFreeFont(nullptr);
        _tft.setTextSize(15);  // Massive gear digit

        _tft.drawString(gear, CP_X + CP_W / 2, MAIN_Y + 80);

        _tft.setTextSize(1);
        _tft.setTextDatum(TL_DATUM);
        _prevGear = gear;
    }

    // ============================================================
    //  Speed — below gear
    // ============================================================
    void _updateSpeed(int speed) {
        if (speed == _prevSpeed) return;
        _clearIdle();

        // Erase old speed
        _tft.fillRect(CP_X + 1, MAIN_Y + 155, CP_W - 2, 40, C_BG);

        String s = String(speed);
        _tft.setTextColor(C_WHITE, C_BG);
        _tft.setTextDatum(MC_DATUM);
        _tft.setTextSize(6);
        _tft.drawString(s, CP_X + CP_W / 2, MAIN_Y + 175);

        _tft.setTextSize(1);
        _tft.setTextDatum(TL_DATUM);
        _prevSpeed = speed;
    }

    // ============================================================
    //  Lap Times — left panel
    // ============================================================
    void _updateLapTimes(const String& cur, const String& last,
                         const String& best, bool lapInv) {
        int baseY = MAIN_Y;

        if (best != _prevBestLap) {
            _tft.fillRect(LP_X + 2, baseY + 20, LP_W - 4, 46, C_BG);
            _lapTimeText(best,  LP_X + 6, baseY + 22, C_CYAN, false);
            _prevBestLap = best;
        }
        if (last != _prevLastLap) {
            _tft.fillRect(LP_X + 2, baseY + 86, LP_W - 4, 46, C_BG);
            _lapTimeText(last,  LP_X + 6, baseY + 88, C_DIM_WHITE, false);
            _prevLastLap = last;
        }
        if (cur != _prevCurLap || lapInv != _prevLapInv) {
            _tft.fillRect(LP_X + 2, baseY + 152, LP_W - 4, 46, C_BG);
            uint16_t col = lapInv ? C_RED : C_WHITE;
            _lapTimeText(cur, LP_X + 6, baseY + 154, col, lapInv);
            _prevCurLap  = cur;
            _prevLapInv  = lapInv;
        }
    }

    void _lapTimeText(const String& t, int x, int y, uint16_t col, bool flash) {
        _tft.setTextColor(col, C_BG);
        _tft.setTextDatum(TL_DATUM);
        _tft.setTextSize(3);
        _tft.drawString(t, x, y);
        _tft.setTextSize(1);
    }

    // ============================================================
    //  Delta — right panel top
    // ============================================================
    void _updateDelta(const String& delta) {
        if (delta == _prevDelta) return;

        _tft.fillRect(RP_X + 1, MAIN_Y + 20, RP_W - 2, 80, C_BG);

        bool negative = (delta.indexOf('-') >= 0);
        uint16_t col  = negative ? C_GREEN : C_RED;  // Negative delta = faster = good

        _tft.setTextColor(col, C_BG);
        _tft.setTextDatum(MC_DATUM);
        _tft.setTextSize(4);
        _tft.drawString(delta, RP_X + RP_W / 2, MAIN_Y + 60);
        _tft.setTextSize(1);
        _tft.setTextDatum(TL_DATUM);
        _prevDelta = delta;
    }

    // ============================================================
    //  Session info — position, lap, fuel
    // ============================================================
    void _updateSessionInfo(int lap, int pos, int cars, float fuel) {
        bool changed = (lap   != _prevLap  ||
                        pos   != _prevPos  ||
                        cars  != _prevCars ||
                        fuel  != _prevFuel);
        if (!changed) return;

        // Position
        if (pos != _prevPos || cars != _prevCars) {
            _tft.fillRect(RP_X + 1, MAIN_Y + 122, RP_W/2 - 2, 26, C_BG);
            _tft.setTextColor(C_WHITE, C_BG);
            _tft.setTextDatum(TL_DATUM);
            _tft.setTextSize(3);
            String posStr = String(pos) + "/" + String(cars);
            _tft.drawString(posStr, RP_X + 4, MAIN_Y + 124);
            _prevPos  = pos;
            _prevCars = cars;
        }

        // Lap number
        if (lap != _prevLap) {
            _tft.fillRect(RP_X + RP_W/2, MAIN_Y + 122, RP_W/2 - 2, 26, C_BG);
            _tft.setTextColor(C_WHITE, C_BG);
            _tft.setTextDatum(TL_DATUM);
            _tft.setTextSize(3);
            _tft.drawString(String(lap), RP_X + RP_W/2 + 4, MAIN_Y + 124);
            _prevLap = lap;
        }

        // Fuel
        if (fuel != _prevFuel) {
            _tft.fillRect(RP_X + 1, MAIN_Y + 162, RP_W - 2, 30, C_BG);
            _tft.setTextDatum(TL_DATUM);
            _tft.setTextSize(3);
            // Color: low fuel warning
            uint16_t col = (fuel < 5.0f) ? C_RED : (fuel < 10.0f ? C_ORANGE : C_GREEN);
            _tft.setTextColor(col, C_BG);
            char buf[16];
            snprintf(buf, sizeof(buf), "%.1fL", fuel);
            _tft.drawString(buf, RP_X + 4, MAIN_Y + 164);
            _prevFuel = fuel;
        }

        _tft.setTextSize(1);
    }

    // ============================================================
    //  Driver Aids — bottom center
    // ============================================================
    void _updateAids(int tc, bool tcA, int abs_, bool absA, float bb) {
        bool changed = (tc != _prevTC || tcA != _prevTCA ||
                        abs_ != _prevABS || absA != _prevABSA ||
                        bb != _prevBB);
        if (!changed) return;

        int x = BT_AID_X;
        int y = BOT_Y + 30;

        // TC
        _tft.fillRect(x + 2,   y - 2,  40, 30, C_BG);
        uint16_t tcCol = tcA ? C_YELLOW : C_MID_GREY;
        _tft.setTextColor(tcCol, C_BG);
        _tft.setTextDatum(TL_DATUM);
        _tft.setTextSize(3);
        _tft.drawString(String(tc), x + 4, y);

        // ABS
        _tft.fillRect(x + 44,  y - 2, 42, 30, C_BG);
        uint16_t absCol = absA ? TFT_CYAN : C_MID_GREY;
        _tft.setTextColor(absCol, C_BG);
        _tft.drawString(String(abs_), x + 46, y);

        // BB
        _tft.fillRect(x + 90,  y - 2, 38, 30, C_BG);
        _tft.setTextColor(C_MAGENTA, C_BG);
        char buf[8];
        snprintf(buf, sizeof(buf), "%.0f", bb);
        _tft.drawString(String(buf), x + 92, y);

        _tft.setTextSize(1);
        _prevTC   = tc;  _prevTCA  = tcA;
        _prevABS  = abs_; _prevABSA = absA;
        _prevBB   = bb;
    }

    // ============================================================
    //  Tyre Data — bottom left
    // ============================================================
    void _updateTyres(float pFL, float pFR, float pRL, float pRR,
                      float tFL, float tFR, float tRL, float tRR) {
        float p[4] = {pFL, pFR, pRL, pRR};
        float t[4] = {tFL, tFR, tRL, tRR};

        // Positions: [FL, FR, RL, RR]
        // Two rows, two columns
        int colX[2] = {BT_TYRE_X + 20, BT_TYRE_X + 102};
        int rowY[2] = {BOT_Y + 28,  BOT_Y + 58};
        int idx[4][2] = {{0,0},{1,0},{0,1},{1,1}};  // [FL,FR,RL,RR] -> [col,row]

        for (int i = 0; i < 4; i++) {
            bool pChanged = (p[i] != _prevTyreP[i]);
            bool tChanged = (t[i] != _prevTyreT[i]);
            if (!pChanged && !tChanged) continue;

            int cx = colX[idx[i][0]];
            int cy = rowY[idx[i][1]];

            _tft.fillRect(cx - 2, cy - 2, 78, 26, C_BG);

            // Pressure in white
            char pbuf[8], tbuf[8];
            snprintf(pbuf, sizeof(pbuf), "%.1f", p[i]);
            snprintf(tbuf, sizeof(tbuf), "%.0f", t[i]);

            // Tyre temp color coding: cold=blue, optimal=green, hot=red
            uint16_t tempCol = _tyreTempColor(t[i]);

            _tft.setTextColor(C_CYAN, C_BG);
            _tft.setTextDatum(TL_DATUM);
            _tft.setTextSize(2);
            _tft.drawString(String(pbuf), cx, cy);

            // Small temp indicator
            _tft.setTextColor(tempCol, C_BG);
            _tft.setTextSize(1);
            _tft.drawString(String(tbuf) + (char)247, cx + 42, cy + 8); // 247 = degree symbol approx

            _prevTyreP[i] = p[i];
            _prevTyreT[i] = t[i];
        }
        _tft.setTextSize(1);
    }

    uint16_t _tyreTempColor(float temp) {
        // Cold < 60°C = blue, optimal 80-110°C = green, hot > 120°C = red
        if (temp < 60)  return TFT_BLUE;
        if (temp < 75)  return TFT_CYAN;
        if (temp < 115) return C_GREEN;
        if (temp < 130) return C_ORANGE;
        return C_RED;
    }

    // ============================================================
    //  Throttle / Brake input bars — bottom right
    // ============================================================
    void _updateInputBars(int thr, int brk) {
        if (thr == _prevThr && brk == _prevBrk) return;

        int x = BT_INP_X + 4;
        int maxW = BT_INP_W - 10;
        int barH = 22;

        // Throttle bar
        if (thr != _prevThr) {
            _tft.fillRect(x, BOT_Y + 15, maxW, barH, C_DARK_GREY);
            int w = (maxW * constrain(thr, 0, 100)) / 100;
            if (w > 0) _tft.fillRect(x, BOT_Y + 15, w, barH, C_GREEN);
            _prevThr = thr;
        }

        // Brake bar
        if (brk != _prevBrk) {
            _tft.fillRect(x, BOT_Y + 50, maxW, barH, C_DARK_GREY);
            int w = (maxW * constrain(brk, 0, 100)) / 100;
            if (w > 0) _tft.fillRect(x, BOT_Y + 50, w, barH, C_RED);
            _prevBrk = brk;
        }
    }

    // ============================================================
    //  Utility
    // ============================================================
    void _labelText(const char* txt, int x, int y, uint16_t col) {
        _tft.setTextColor(col, C_BG);
        _tft.setTextDatum(TL_DATUM);
        _tft.setTextSize(1);
        _tft.drawString(txt, x, y);
    }
};
