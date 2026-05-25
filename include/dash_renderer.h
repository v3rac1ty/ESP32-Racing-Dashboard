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

        // Allocate the scrolling graph sprite (rendered in RAM, pushed as one SPI burst)
        _graphSprite = new TFT_eSprite(&_tft);
        _graphSprite->setColorDepth(16);
        _graphSprite->createSprite(GRAPH_W, GRAPH_H);
        _graphSprite->fillSprite(C_BG);
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
        _updateSectors(d.s1Time, d.s2Time, d.s3Time, d.s1Flag, d.s2Flag, d.s3Flag);
        _updateDelta(d.delta);
        _updateSessionInfo(d.lap, d.position, d.numCars, d.fuel);
        _updateAids(d.tcLevel, d.tcActive, d.absLevel, d.absActive, d.brakeBias,
                    d.diffOnThrottle);
        _updateTyres(d.tyrePFL, d.tyrePFR, d.tyrePRL, d.tyrePRR,
                     d.tyreTFL, d.tyreTFR, d.tyreTRL, d.tyreTRR);
        _updateGraph(d.throttle, d.brake);
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
    String _prevS1Time, _prevS2Time, _prevS3Time;
    int    _prevS1Flag = -1, _prevS2Flag = -1, _prevS3Flag = -1;
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
    int    _prevDiffOn    = -1;
    int    _prevDiffOff   = -1;
    float  _prevTyreP[4]  = {-1,-1,-1,-1};
    float  _prevTyreT[4]  = {-1,-1,-1,-1};
    bool   _idleCleared   = false;

    // ---- Scrolling trace graph ----
    TFT_eSprite* _graphSprite = nullptr;
    uint8_t _gthr[GRAPH_W]   = {};   // ring buffer — throttle 0-100
    uint8_t _gbrk[GRAPH_W]   = {};   // ring buffer — brake    0-100
    int     _ghead            = 0;   // next write position

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
        // Lap times — tighter spacing to leave room for sector strip at bottom
        _labelText("BEST",    LP_X + 4,  MAIN_Y + 4,   C_CYAN);
        _labelText("LAST",    LP_X + 4,  MAIN_Y + 44,  C_DIM_WHITE);
        _labelText("CURRENT", LP_X + 4,  MAIN_Y + 84,  C_WHITE);

        // Sector strip separator and labels (static S1/S2/S3 headers)
        // Sector area: MAIN_Y+124 → MAIN_Y+200
        _tft.drawFastHLine(LP_X, MAIN_Y + 122, LP_W, C_DARK_GREY);
        _labelText("SECTORS", LP_X + 4,  MAIN_Y + 126, C_LIGHT_GREY);
        // Each box is 50px wide with 2px gap; label centered in box
        _labelText("S1",      LP_X + 19, MAIN_Y + 140, C_LIGHT_GREY);
        _labelText("S2",      LP_X + 71, MAIN_Y + 140, C_LIGHT_GREY);
        _labelText("S3",      LP_X + 119,MAIN_Y + 140, C_LIGHT_GREY);

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

        // ---- Bottom aids — row 1: TC / ABS ----
        _labelText("TC",    BT_AID_X + 4,  BOT_Y + 4,  C_YELLOW);
        _labelText("ABS",   BT_AID_X + 68, BOT_Y + 4,  C_BLUE);
        // ---- Bottom aids — row 2: BB / DIFF ----
        _labelText("BB",    BT_AID_X + 4,  BOT_Y + 42, C_MAGENTA);
        _labelText("DIFF",  BT_AID_X + 68, BOT_Y + 42, C_TEAL);

        // ---- Trace graph border and label ----
        _tft.drawRect(GRAPH_X - 1, GRAPH_Y - 1, GRAPH_W + 2, GRAPH_H + 2, C_DARK_GREY);
        _labelText("TRACE", GRAPH_X, BOT_Y - 2 + BOT_H - 6, C_MID_GREY);  // tiny label below border

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
            _tft.fillRect(LP_X + 2, baseY + 14, LP_W - 4, 26, C_BG);
            _lapTimeText(best,  LP_X + 4, baseY + 15, C_CYAN, false);
            _prevBestLap = best;
        }
        if (last != _prevLastLap) {
            _tft.fillRect(LP_X + 2, baseY + 54, LP_W - 4, 26, C_BG);
            _lapTimeText(last,  LP_X + 4, baseY + 55, C_DIM_WHITE, false);
            _prevLastLap = last;
        }
        if (cur != _prevCurLap || lapInv != _prevLapInv) {
            _tft.fillRect(LP_X + 2, baseY + 94, LP_W - 4, 26, C_BG);
            uint16_t col = lapInv ? C_RED : C_WHITE;
            _lapTimeText(cur, LP_X + 4, baseY + 95, col, lapInv);
            _prevCurLap  = cur;
            _prevLapInv  = lapInv;
        }
    }

    void _lapTimeText(const String& t, int x, int y, uint16_t col, bool flash) {
        _tft.setTextColor(col, C_BG);
        _tft.setTextDatum(TL_DATUM);
        _tft.setTextSize(2);
        _tft.drawString(t, x, y);
        _tft.setTextSize(1);
    }

    // ============================================================
    //  Sector Times — 3 boxes in the bottom of the left panel
    //  MAIN_Y+152 to MAIN_Y+200
    //  Each box 48px wide, 44px tall; boxes at x=2, 52, 102
    //  Purple fill for session best; colored text otherwise.
    // ============================================================
    void _updateSectors(const String& s1, const String& s2, const String& s3,
                        int f1, int f2, int f3) {
        bool changed = (s1 != _prevS1Time || f1 != _prevS1Flag ||
                        s2 != _prevS2Time || f2 != _prevS2Flag ||
                        s3 != _prevS3Time || f3 != _prevS3Flag);
        if (!changed) return;

        static constexpr int BOX_X[3] = {LP_X + 2,  LP_X + 52, LP_X + 104};
        static constexpr int BOX_W    = 48;
        static constexpr int BOX_Y    = MAIN_Y + 152;
        static constexpr int BOX_H    = 46;

        const String* times[3] = {&s1, &s2, &s3};
        int           flags[3] = {f1, f2, f3};
        bool          prevChanged[3] = {
            s1 != _prevS1Time || f1 != _prevS1Flag,
            s2 != _prevS2Time || f2 != _prevS2Flag,
            s3 != _prevS3Time || f3 != _prevS3Flag
        };

        for (int i = 0; i < 3; i++) {
            if (!prevChanged[i]) continue;

            int   bx   = BOX_X[i];
            int   flag = flags[i];
            uint16_t col = sectorColor(flag);

            // Background: purple fill for session best, dark for others
            if (flag == 3) {
                _tft.fillRect(bx, BOX_Y, BOX_W, BOX_H, C_PURPLE);
                _tft.drawRect(bx, BOX_Y, BOX_W, BOX_H, C_PURPLE);
            } else {
                _tft.fillRect(bx, BOX_Y, BOX_W, BOX_H, C_BG);
                _tft.drawRect(bx, BOX_Y, BOX_W, BOX_H, C_DARK_GREY);
            }

            // Time value — centered in box
            uint16_t textCol = (flag == 3) ? C_WHITE : col;
            _tft.setTextColor(textCol, (flag == 3) ? C_PURPLE : C_BG);
            _tft.setTextDatum(MC_DATUM);
            _tft.setTextSize(1);
            _tft.drawString(*times[i], bx + BOX_W / 2, BOX_Y + BOX_H / 2);
        }

        _tft.setTextDatum(TL_DATUM);
        _prevS1Time = s1;  _prevS1Flag = f1;
        _prevS2Time = s2;  _prevS2Flag = f2;
        _prevS3Time = s3;  _prevS3Flag = f3;
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
    //  Driver Aids — 2×2 grid:
    //  Row 1:  TC       ABS
    //  Row 2:  BB       DIFF (on-throttle)
    // ============================================================
    void _updateAids(int tc, bool tcA, int abs_, bool absA, float bb, int diffOn) {
        bool changed = (tc    != _prevTC   || tcA  != _prevTCA  ||
                        abs_  != _prevABS  || absA != _prevABSA ||
                        bb    != _prevBB   || diffOn != _prevDiffOn);
        if (!changed) return;

        int x  = BT_AID_X;
        int y1 = BOT_Y + 18;   // row 1 values
        int y2 = BOT_Y + 56;   // row 2 values

        _tft.setTextDatum(TL_DATUM);
        _tft.setTextSize(3);

        // ── Row 1, Col 1: TC ──
        _tft.fillRect(x + 2,  y1 - 2, 60, 26, C_BG);
        _tft.setTextColor(tcA ? C_YELLOW : C_MID_GREY, C_BG);
        _tft.drawString(String(tc), x + 4, y1);

        // ── Row 1, Col 2: ABS ──
        _tft.fillRect(x + 66, y1 - 2, 60, 26, C_BG);
        _tft.setTextColor(absA ? TFT_CYAN : C_MID_GREY, C_BG);
        _tft.drawString(String(abs_), x + 68, y1);

        // ── Row 2, Col 1: BB ──
        _tft.fillRect(x + 2,  y2 - 2, 60, 26, C_BG);
        _tft.setTextColor(C_MAGENTA, C_BG);
        char buf[8];
        snprintf(buf, sizeof(buf), "%.0f", bb);
        _tft.drawString(buf, x + 4, y2);

        // ── Row 2, Col 2: DIFF (on-throttle) ──
        _tft.fillRect(x + 66, y2 - 2, 62, 26, C_BG);
        _tft.setTextColor(C_TEAL, C_BG);
        snprintf(buf, sizeof(buf), "%d%%", diffOn);
        _tft.drawString(buf, x + 68, y2);

        _tft.setTextSize(1);
        _prevTC     = tc;   _prevTCA    = tcA;
        _prevABS    = abs_; _prevABSA   = absA;
        _prevBB     = bb;   _prevDiffOn = diffOn;
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
    //  Scrolling throttle / brake trace — bottom right
    //
    //  Strategy: ring buffer (140 uint8 values each) rendered into
    //  a TFT_eSprite in RAM, then pushed as a single SPI burst.
    //  This scrolls smoothly at SimHub's update rate (~20-30 Hz)
    //  without reading pixels back from the slow SPI bus.
    //
    //  Layout (from bottom = 0%):
    //    Green columns = throttle  (fills upward)
    //    Red   columns = brake     (fills upward, drawn on top)
    //  A light 25% / 50% / 75% grid gives depth.
    // ============================================================
    void _updateGraph(int thr, int brk) {
        thr = constrain(thr, 0, 100);
        brk = constrain(brk, 0, 100);

        // Push new sample into ring buffer
        _gthr[_ghead] = (uint8_t)thr;
        _gbrk[_ghead] = (uint8_t)brk;
        _ghead = (_ghead + 1) % GRAPH_W;

        // ── Render sprite ──
        _graphSprite->fillSprite(C_BG);

        // Subtle horizontal grid lines at 25 / 50 / 75 %
        static constexpr uint16_t GRID = 0x1082;   // very dark grey
        static const int gridPct[3] = {25, 50, 75};
        for (int gi = 0; gi < 3; gi++) {
            int gy = GRAPH_H - (gridPct[gi] * GRAPH_H) / 100;
            _graphSprite->drawFastHLine(0, gy, GRAPH_W, GRID);
        }

        // Draw each column oldest-to-newest (left = past, right = present)
        // Connect consecutive points with a vertical segment to avoid gaps on fast moves.
        for (int col = 0; col < GRAPH_W; col++) {
            int i  = (_ghead + col) % GRAPH_W;
            int tv = _gthr[i];
            int bv = _gbrk[i];

            // y position: 0% → bottom pixel, 100% → top pixel
            int thrY = GRAPH_H - 1 - (tv * (GRAPH_H - 1)) / 100;
            int brkY = GRAPH_H - 1 - (bv * (GRAPH_H - 1)) / 100;

            if (col == 0) {
                // First column — single pixel each
                _graphSprite->drawPixel(0, thrY, C_GREEN);
                _graphSprite->drawPixel(0, brkY, C_RED);
            } else {
                int pi      = (_ghead + col - 1) % GRAPH_W;
                int prevThrY = GRAPH_H - 1 - (_gthr[pi] * (GRAPH_H - 1)) / 100;
                int prevBrkY = GRAPH_H - 1 - (_gbrk[pi] * (GRAPH_H - 1)) / 100;

                // Throttle line — fill vertical gap so diagonal moves stay connected
                int minY = thrY < prevThrY ? thrY : prevThrY;
                int maxY = thrY > prevThrY ? thrY : prevThrY;
                _graphSprite->drawFastVLine(col, minY, maxY - minY + 1, C_GREEN);

                // Brake line — same treatment
                minY = brkY < prevBrkY ? brkY : prevBrkY;
                maxY = brkY > prevBrkY ? brkY : prevBrkY;
                _graphSprite->drawFastVLine(col, minY, maxY - minY + 1, C_RED);
            }
        }

        // Push sprite to screen as one SPI burst
        _graphSprite->pushSprite(GRAPH_X, GRAPH_Y);
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
