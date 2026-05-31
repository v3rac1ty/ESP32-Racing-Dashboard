#pragma once
#include <TFT_eSPI.h>
#include "dashboard.h"
#include "simhub_protocol.h"

// ============================================================
//  DashRenderer — F1 25 PFM21 exact replica on ILI9488 480×320
//  All positions verified to stay within MAIN_H=200 budget.
//
//  Left panel Y offsets from MAIN_Y (48):
//    +0  L{lap} P{pos}/{cars}     textSize 1
//    +10 Current lap              textSize 2 (16px)
//    +27 "CURRENT" label          textSize 1
//    +36 Last lap                 textSize 2
//    +53 "LAST" + delta           textSize 1
//    +62 Best lap                 textSize 2
//    +79 "BEST"                   textSize 1
//    +88 separator + "DELTA"
//    +98 Delta value              textSize 3 (24px)
//   +124 Fuel line                textSize 1
//   +133 F/B gap line             textSize 1
//   +146 S1                        textSize 1
//   +158 S2                        textSize 1
//   +170 S3                        textSize 1
//   (+182 max used → 230px < 248px MAIN_H) ✓
// ============================================================
class DashRenderer {
public:
    explicit DashRenderer(TFT_eSPI& tft) : _tft(tft) {}

    void begin() {
        _tft.begin();
        _tft.setRotation(1);
        _tft.fillScreen(C_BG);

        _traceSprite = new TFT_eSprite(&_tft);
        _traceSprite->setColorDepth(16);
        _traceSprite->createSprite(TRACE_W, TRACE_H);
        _traceSprite->fillSprite(C_BG);

        _drawChrome();
        _drawIdle();
    }

    void update(const SimHubData& d) {
        if (!d.valid) return;
        if (!_idleCleared) { _clearIdle(); _idleCleared = true; }

        _updateRpm(d.rpmPct, d.rpmRedline);
        _updateErsModeBadge(d.ersMode);
        _updateAids(d.tcLevel, d.tcActive, d.absLevel, d.absActive);
        _updatePos(d.position, d.numCars, d.lap);
        _updateLapTimes(d.curLap, d.lastLap, d.bestLap,
                        d.lapInvalid, d.lastLapDelta, d.delta);
        _updateFuelGap(d.fuel, d.gapFront, d.gapBehind);
        _updateSectors(d.s1Time, d.s2Time, d.s3Time,
                       d.s1Flag, d.s2Flag, d.s3Flag);
        _updateCenterPanel(d);
        _updateTyres(d.tyreTFL, d.tyreTFR, d.tyreTRL, d.tyreTRR,
                     d.tyreWearFL, d.tyreWearFR, d.tyreWearRL, d.tyreWearRR,
                     d.tyreCompound);
        _updateDrs(d.drsStatus);
        _updateSetup(d.brakeBias, d.diffOnThrottle);
        _updateErsBar(d.ersPct);
        _updateTrace(d.throttle, d.brake);
    }

private:
    TFT_eSPI&    _tft;
    TFT_eSprite* _traceSprite = nullptr;

    // ── Throttle/brake ring buffers ──────────────────────────
    uint8_t _thrBuf[TRACE_W] = {};
    uint8_t _brkBuf[TRACE_W] = {};
    int     _trHead = 0;

    // ── Dirty-check caches ───────────────────────────────────
    int    _cRpm = -1, _cRed = -1;
    int    _cErsMode = -2;
    int    _cSpeed = -1;
    int    _cDrs = -1;
    float  _cBB = -999.0f;
    int    _cDiff = -1;
    int    _cPos = -1, _cCars = -1, _cLap = -1;
    String _cCurLap, _cLastLap, _cBestLap;
    bool   _cLapInv = false;
    String _cLastDelta, _cDelta;
    String _cGear;
    float  _cTyreT[4] = {-1,-1,-1,-1};
    int    _cTyreW[4] = {-1,-1,-1,-1};
    String _cComp;
    int    _cTC = -1, _cABS = -1;
    bool   _cTCA = false, _cABSA = false;
    String _cS1, _cS2, _cS3;
    int    _cF1 = -1, _cF2 = -1, _cF3 = -1;
    int    _cErsPct = -1;
    float  _cFuel  = -1.0f;
    float  _cGapF  = -1.0f, _cGapB = -1.0f;
    int    _cOverlay = -1;
    float  _cScReq = -999.0f;
    String _cScDelta;
    int    _cPitSpeed = -1;
    int    _cPitLimit = -1;
    bool   _idleCleared = false;

    static constexpr int CP_SPEED_STRIP_H = 28;
    static constexpr int CP_SPEED_CLEAR_Y = MAIN_Y + MAIN_H - CP_SPEED_STRIP_H;
    static constexpr int CP_GEAR_CLEAR_Y  = MAIN_Y + 18;
    static constexpr int CP_GEAR_CLEAR_H  = CP_SPEED_CLEAR_Y - CP_GEAR_CLEAR_Y;

    static constexpr int OVL_BOX_W = CP_W - 20;
    static constexpr int OVL_BOX_H = 60;
    static constexpr int OVL_BOX_X = CP_X + 10;
    static constexpr int OVL_BOX_Y = MAIN_Y + (MAIN_H - OVL_BOX_H) / 2;
    static constexpr int OVL_TITLE_Y = OVL_BOX_Y + 14;
    static constexpr int OVL_LINE1_Y = OVL_BOX_Y + 34;
    static constexpr int OVL_LINE2_Y = OVL_BOX_Y + 48;

    // ============================================================
    //  Static chrome — drawn once in begin()
    // ============================================================
    void _drawChrome() {
        // Horizontal separators
        _tft.drawFastHLine(0, SEP1_Y,  SCR_W, C_DARK_GREY);
        _tft.drawFastHLine(0, SEP2_Y,  SCR_W, C_MID_GREY);
        _tft.drawFastHLine(0, SEP3_Y,  SCR_W, C_MID_GREY);

        // Main-body vertical dividers
        _tft.drawFastVLine(CP_X, MAIN_Y, MAIN_H, C_DARK_GREY);
        _tft.drawFastVLine(RP_X, MAIN_Y, MAIN_H, C_DARK_GREY);

        // Info-bar vertical dividers
        const int divs[] = {IB_TC_X, IB_ABS_X, IB_POS_X};
        for (int x : divs) _tft.drawFastVLine(x, IB_Y, IB_H, C_DARK_GREY);

        // TC badge (static yellow left-half)
        _tft.fillRect(IB_TC_X,  IB_Y, 14, IB_H, C_YELLOW);
        _tft.setTextColor(C_BG, C_YELLOW);
        _tft.setTextDatum(MC_DATUM); _tft.setTextSize(1);
        _tft.drawString("T", IB_TC_X + 7, IB_Y + IB_H/2);

        // ABS badge (static blue left-half)
        _tft.fillRect(IB_ABS_X, IB_Y, 14, IB_H, C_DODGER_BLUE);
        _tft.setTextColor(C_WHITE, C_DODGER_BLUE);
        _tft.drawString("A", IB_ABS_X + 7, IB_Y + IB_H/2);
        _tft.setTextDatum(TL_DATUM); _tft.setTextSize(1);

        // RPM dot outlines
        _drawRpmRing(-1);

        // Left panel labels (below/above their values)
        _label("CURRENT", LP_X+2,  MAIN_Y+27, C_GREEN);
        _label("LAST",    LP_X+2,  MAIN_Y+53, C_MID_GREY);
        _label("BEST",    LP_X+2,  MAIN_Y+79, C_MAGENTA);
        _tft.drawFastHLine(LP_X, MAIN_Y+88, LP_W, C_DARK_GREY);
        _label("DELTA",   LP_X+2,  MAIN_Y+90, C_LIGHT_GREY);

        // Right panel labels
        _label("FL",  RP_X+4,    MAIN_Y+58,  C_LIGHT_GREY);
        _label("FR",  RP_X+78,   MAIN_Y+58,  C_LIGHT_GREY);
        _label("RL",  RP_X+4,    MAIN_Y+110, C_LIGHT_GREY);
        _label("RR",  RP_X+78,   MAIN_Y+110, C_LIGHT_GREY);
        _label("B",   RP_X+4,    MAIN_Y+168, C_LIGHT_GREY);
        _label("D",   RP_X+78,   MAIN_Y+168, C_LIGHT_GREY);
    }

    void _drawIdle() {
        _tft.setTextColor(C_MID_GREY, C_BG);
        _tft.setTextDatum(MC_DATUM); _tft.setTextSize(2);
        _tft.drawString("Waiting for", CP_X + CP_W/2, MAIN_Y + MAIN_H/2 - 10);
        _tft.drawString("SimHub...",   CP_X + CP_W/2, MAIN_Y + MAIN_H/2 + 12);
        _tft.setTextSize(1); _tft.setTextDatum(TL_DATUM);
    }
    void _clearIdle() {
        _tft.fillRect(CP_X+1, MAIN_Y+18, CP_W-2, MAIN_H-20, C_BG);
    }

    // ============================================================
    //  RPM LED strip — 24 dots, only repaint changed dots
    // ============================================================
    void _drawRpmRing(int litCount) {
        static constexpr int SLOT = SCR_W / RPM_DOTS;
        static constexpr int CY   = RPM_Y + RPM_H/2;
        int redDot = (_cRed >= 0) ? (_cRed * RPM_DOTS + 50) / 100 : RPM_DOTS;
        for (int i = 0; i < RPM_DOTS; i++) {
            bool isLit = (i < litCount);
            int cx = SLOT/2 + i*SLOT;
            if (isLit) {
                uint16_t col = (i >= redDot) ? C_RED : rpmDotColor(i);
                _tft.fillCircle(cx, CY, RPM_R, col);
            } else {
                _tft.fillCircle(cx, CY, RPM_R, C_BG);
                _tft.drawCircle(cx, CY, RPM_R, C_DARK_GREY);
            }
        }
    }

    void _updateRpm(int pct, int redline) {
        if (pct == _cRpm && redline == _cRed) return;
        pct     = constrain(pct,     0, 100);
        redline = constrain(redline, 1, 100);
        int litNow  = (pct  * RPM_DOTS + 50) / 100;
        int litPrev = (_cRpm < 0) ? -1 : (_cRpm * RPM_DOTS + 50) / 100;
        int redDot  = (redline * RPM_DOTS + 50) / 100;

        static constexpr int SLOT = SCR_W / RPM_DOTS;
        static constexpr int CY   = RPM_Y + RPM_H/2;

        for (int i = 0; i < RPM_DOTS; i++) {
            bool wasLit = (i < litPrev);
            bool isLit  = (i < litNow);
            if (wasLit == isLit && redline == _cRed) continue;
            int cx = SLOT/2 + i*SLOT;
            if (isLit) {
                uint16_t col = (i >= redDot) ? C_RED : rpmDotColor(i);
                _tft.fillCircle(cx, CY, RPM_R, col);
            } else {
                _tft.fillCircle(cx, CY, RPM_R, C_BG);
                _tft.drawCircle(cx, CY, RPM_R, C_DARK_GREY);
            }
        }
        _cRpm = pct; _cRed = redline;
    }

    // ============================================================
    //  ERS Mode badge — info bar left cell (colored bg + text)
    //  0=NONE/Firebrick  1=MEDIUM/Goldenrod
    //  2=HOT LAP/ForestGreen  3=OVERTAKE/LimeGreen
    // ============================================================
    void _updateErsModeBadge(int mode) {
        if (mode == _cErsMode) return;
        uint16_t bg = ersModeColor(mode);
        const char* lbl = ersModeLabel(mode);
        _tft.fillRect(IB_ERS_X, IB_Y, IB_ERS_W, IB_H, bg);
        _tft.setTextColor(C_WHITE, bg);
        _tft.setTextDatum(MC_DATUM); _tft.setTextSize(1);
        _tft.drawString(lbl, IB_ERS_X + IB_ERS_W/2, IB_Y + IB_H/2);
        _tft.setTextDatum(TL_DATUM);
        _cErsMode = mode;
    }

    // ============================================================
    //  Speed — center panel, below gear
    // ============================================================
    void _updateSpeed(int spd) {
        if (spd == _cSpeed) return;
        const int SPD_Y = CP_SPEED_CLEAR_Y + CP_SPEED_STRIP_H/2;
        // Fixed 3-char width ("%3d") keeps the MC_DATUM bounding box constant
        // (3 × 6 × textSize3 = 54 px) so drawString with bg colour overwrites
        // cleanly without a prior fillRect — no black-flash flicker.
        char buf[6]; snprintf(buf, sizeof(buf), "%3d", spd);
        _tft.setTextColor(C_WHITE, C_BG);
        _tft.setTextDatum(MC_DATUM); _tft.setTextSize(3);
        _tft.drawString(buf, CP_X + CP_W/2, SPD_Y);
        _tft.setTextSize(1); _tft.setTextDatum(ML_DATUM);
        _tft.drawString("KM/H", CP_X + CP_W/2 + 44, SPD_Y);
        _tft.setTextDatum(TL_DATUM);
        _cSpeed = spd;
    }

    // ============================================================
    //  DRS icon — right panel, above tyre telemetry
    // ============================================================
    void _updateDrs(int status) {
        if (status == _cDrs) return;
        static constexpr int DRS_X = RP_X + 2;
        static constexpr int DRS_Y = MAIN_Y + 18;
        static constexpr int DRS_W = RP_W - 4;
        static constexpr int DRS_H = 16;
        uint16_t bg, fg;
        const char* lbl;
        switch (status) {
            case 2:  bg = C_GREEN;     fg = C_BG;       lbl = "DRS ACTIVE";  break;
            case 1:  bg = C_DARK_GREY; fg = C_YELLOW;   lbl = "DRS AVAIL";   break;
            case 3:  bg = C_ERS_NONE;  fg = C_WHITE;    lbl = "DRS DISABLED";break;
            default: bg = C_DARK_GREY; fg = C_MID_GREY; lbl = "DRS";         break;
        }
        _tft.fillRect(DRS_X, DRS_Y, DRS_W, DRS_H, bg);
        _tft.drawRect(DRS_X, DRS_Y, DRS_W, DRS_H, C_DARK_GREY);
        _tft.setTextColor(fg, bg);
        _tft.setTextDatum(MC_DATUM); _tft.setTextSize(1);
        _tft.drawString(lbl, DRS_X + DRS_W/2, DRS_Y + DRS_H/2);
        _tft.setTextDatum(TL_DATUM);
        _cDrs = status;
    }

    // ============================================================
    //  Race Position — info bar right + lap/pos line in left panel
    //  Yellow "P{n}" mirroring WYS1 RacePositionText0
    // ============================================================
    void _updatePos(int pos, int cars, int lap) {
        if (pos == _cPos && cars == _cCars && lap == _cLap) return;

        // Info bar: yellow P{n}
        _tft.fillRect(IB_POS_X, IB_Y, IB_POS_W, IB_H, C_BG);
        char pb[5]; snprintf(pb, sizeof(pb), "P%d", pos);
        _tft.setTextColor(C_YELLOW, C_BG);
        _tft.setTextDatum(MC_DATUM); _tft.setTextSize(2);
        _tft.drawString(pb, IB_POS_X + IB_POS_W/2, IB_Y + IB_H/2);
        _tft.setTextDatum(TL_DATUM); _tft.setTextSize(1);

        // Left panel top: L{n} / {pos}/{cars}
        _tft.fillRect(LP_X, MAIN_Y, LP_W, 10, C_BG);
        char lb[16]; snprintf(lb, sizeof(lb), "L%d  %d/%d", lap, pos, cars);
        _tft.setTextColor(C_LIGHT_GREY, C_BG);
        _tft.setTextSize(1);
        _tft.setCursor(LP_X+2, MAIN_Y+1);
        _tft.print(lb);

        _cPos = pos; _cCars = cars; _cLap = lap;
    }

    // ============================================================
    //  Lap Times — left panel, matching WYS1 color scheme
    //  Current = green (#00FF00)
    //  Last    = white (#FFFFFF)
    //  Best    = magenta (#FF00FF)
    //
    //  Offsets from MAIN_Y:
    //   +10  Current value  (textSize 2, 16px)
    //   +27  CURRENT label
    //   +36  Last value     (textSize 2)
    //   +53  LAST label + sub-delta
    //   +62  Best value     (textSize 2)
    //   +79  BEST label
    //   +88  separator
    //   +90  DELTA label
    //   +98  Delta value    (textSize 3, 24px)
    // ============================================================
    void _updateLapTimes(const String& cur, const String& last,
                         const String& best, bool lapInv,
                         const String& lastDelta, const String& delta) {
        // ── Current lap (green, or red when invalid) ──────────
        if (cur != _cCurLap || lapInv != _cLapInv) {
            _tft.fillRect(LP_X, MAIN_Y+10, LP_W, 17, C_BG);
            uint16_t col = lapInv ? C_RED : C_GREEN;
            _tft.setTextColor(col, C_BG);
            _tft.setTextDatum(TL_DATUM); _tft.setTextSize(2);
            _tft.drawString(cur, LP_X+2, MAIN_Y+10);
            _tft.setTextSize(1);
            _cCurLap = cur; _cLapInv = lapInv;
        }

        // ── Last lap (white) ──────────────────────────────────
        if (last != _cLastLap || lastDelta != _cLastDelta) {
            _tft.fillRect(LP_X, MAIN_Y+36, LP_W, 17, C_BG);
            _tft.setTextColor(C_WHITE, C_BG);
            _tft.setTextDatum(TL_DATUM); _tft.setTextSize(2);
            _tft.drawString(last, LP_X+2, MAIN_Y+36);
            // Sub-delta right-aligned below LAST label
            if (!lastDelta.isEmpty() && lastDelta != "---") {
                bool neg = (lastDelta[0] == '-');
                _tft.setTextColor(neg ? C_GREEN : C_RED, C_BG);
                _tft.setTextDatum(TR_DATUM); _tft.setTextSize(1);
                _tft.drawString(lastDelta, LP_X+LP_W-2, MAIN_Y+54);
                _tft.setTextDatum(TL_DATUM);
            }
            _tft.setTextSize(1);
            _cLastLap = last; _cLastDelta = lastDelta;
        }

        // ── Best lap (magenta) ────────────────────────────────
        if (best != _cBestLap) {
            _tft.fillRect(LP_X, MAIN_Y+62, LP_W, 17, C_BG);
            _tft.setTextColor(C_MAGENTA, C_BG);
            _tft.setTextDatum(TL_DATUM); _tft.setTextSize(2);
            _tft.drawString(best, LP_X+2, MAIN_Y+62);
            _tft.setTextSize(1);
            _cBestLap = best;
        }

        // ── Delta to session best (red=slower, green=faster) ──
        if (delta != _cDelta) {
            _tft.fillRect(LP_X, MAIN_Y+98, LP_W, 25, C_BG);
            bool neg = (!delta.isEmpty() && delta[0] == '-');
            _tft.setTextColor(neg ? C_GREEN : C_RED, C_BG);
            _tft.setTextDatum(MC_DATUM); _tft.setTextSize(3);
            _tft.drawString(delta, LP_X + LP_W/2, MAIN_Y+110);
            _tft.setTextSize(1); _tft.setTextDatum(TL_DATUM);
            _cDelta = delta;
        }
    }

    // ============================================================
    //  Fuel + Gaps — left panel bottom rows (compact, 1 line each)
    //  All within MAIN_Y+124..+141 → absolute Y=172..189 < 248 ✓
    // ============================================================
    void _updateFuelGap(float fuel, float gapF, float gapB) {
        if (fuel != _cFuel) {
            _tft.fillRect(LP_X, MAIN_Y+124, LP_W, 9, C_BG);
            uint16_t col = (fuel < 3.f) ? C_RED :
                           (fuel < 8.f ? C_ORANGE : C_GREEN);
            char buf[10]; snprintf(buf, sizeof(buf), "FUEL %.1fL", fuel);
            _tft.setTextColor(col, C_BG); _tft.setTextSize(1);
            _tft.setCursor(LP_X+2, MAIN_Y+124); _tft.print(buf);
            _cFuel = fuel;
        }
        auto fmt = [](float g) -> String {
            if (g <= 0.0005f || g > 59.9f) return String("---.---");
            char b[9]; snprintf(b, sizeof(b), "%.3f", g); return String(b);
        };
        if (gapF != _cGapF || gapB != _cGapB) {
            _tft.fillRect(LP_X, MAIN_Y+133, LP_W, 9, C_BG);
            // Gap ahead (red) left-half, gap behind (green) right-half
            _tft.setTextSize(1);
            _tft.setTextColor(C_RED, C_BG);
            _tft.setCursor(LP_X+2, MAIN_Y+133); _tft.print(fmt(gapF));
            _tft.setTextColor(C_GREEN, C_BG);
            _tft.setCursor(LP_X+LP_W/2+2, MAIN_Y+133); _tft.print(fmt(gapB));
            _cGapF = gapF; _cGapB = gapB;
        }
    }

    // ============================================================
    //  Center panel — gear/speed or overlay (SC/VSC/Pit)
    // ============================================================
    void _updateCenterPanel(const SimHubData& d) {
        int overlay = 0;
        if (d.pitLimiterActive) overlay = 3;
        else if (d.safetyCarStatus == 1) overlay = 1;
        else if (d.safetyCarStatus == 2) overlay = 2;

        bool overlayChanged = (overlay != _cOverlay);
        if (overlayChanged) {
            _clearCenterPanel();
            _cGear = "";
            _cSpeed = -1;
            _cScReq = -999.f;
            _cScDelta = "";
            _cPitSpeed = -1;
            _cPitLimit = -1;
        }

        if (overlay == 0) {
            _updateGear(d.gear);
            _updateSpeed(d.speed);
            _cOverlay = 0;
            return;
        }

        if (overlay == 3) {
            if (overlayChanged) {
                _drawOverlayFrame(C_DODGER_BLUE, C_WHITE, "PIT LIMITER");
            }
            if (d.pitSpeedLimit != _cPitLimit) {
                String line1 = String("LIMIT ") + String(d.pitSpeedLimit) + " KM/H";
                _updateOverlayLine(OVL_LINE1_Y, line1, C_WHITE, C_DODGER_BLUE);
                _cPitLimit = d.pitSpeedLimit;
            }
            if (d.speed != _cPitSpeed) {
                String line2 = String(d.speed);
                _updateOverlayLine(OVL_LINE2_Y, line2, C_WHITE, C_DODGER_BLUE);
                _cPitSpeed = d.speed;
            }
        } else {
            uint16_t bg = (overlay == 1) ? C_YELLOW : C_ORANGE;
            const char* title = (overlay == 1) ? "SAFETY CAR" : "VIRTUAL SAFETY";
            if (overlayChanged) {
                _drawOverlayFrame(bg, C_BG, title);
            }
            if (d.scRequiredDelta != _cScReq) {
                char req[20];
                snprintf(req, sizeof(req), "REQ %+.3f s", d.scRequiredDelta);
                _updateOverlayLine(OVL_LINE1_Y, String(req), C_BG, bg);
                _cScReq = d.scRequiredDelta;
            }
            String cur = d.delta.isEmpty() ? String("---") : d.delta;
            if (cur != _cScDelta) {
                _updateOverlayLine(OVL_LINE2_Y, cur, C_BG, bg);
                _cScDelta = cur;
            }
        }
        _cOverlay = overlay;
    }

    void _clearCenterPanel() {
        _tft.fillRect(CP_X+1, MAIN_Y+18, CP_W-2, MAIN_H-20, C_BG);
    }

    void _drawOverlayFrame(uint16_t bg, uint16_t fg, const char* title) {
        _tft.fillRect(OVL_BOX_X, OVL_BOX_Y, OVL_BOX_W, OVL_BOX_H, bg);
        _tft.drawRect(OVL_BOX_X, OVL_BOX_Y, OVL_BOX_W, OVL_BOX_H, C_BG);
        _tft.setTextColor(fg, bg);
        _tft.setTextDatum(MC_DATUM);
        _tft.setTextSize(2);
        _tft.drawString(title, OVL_BOX_X + OVL_BOX_W/2, OVL_TITLE_Y);
        _tft.setTextDatum(TL_DATUM); _tft.setTextSize(1);
    }

    void _updateOverlayLine(int y, const String& text, uint16_t fg, uint16_t bg) {
        _tft.fillRect(OVL_BOX_X+2, y-6, OVL_BOX_W-4, 12, bg);
        _tft.setTextColor(fg, bg);
        _tft.setTextDatum(MC_DATUM); _tft.setTextSize(1);
        _tft.drawString(text, OVL_BOX_X + OVL_BOX_W/2, y);
        _tft.setTextDatum(TL_DATUM); _tft.setTextSize(1);
    }

    // ============================================================
    //  Gear — center panel, dominant element
    //  WYS1: Roboto ExtraBlack size 1125 in a 686×850 area
    //  ESP32: textSize 14 (~84×112px) centered in 200×182px
    //  White (1-8, N), Red (R)
    // ============================================================
    void _updateGear(const String& gear) {
        if (gear == _cGear) return;
        // No fillRect: GLCD font chars are all 6 columns wide at any textSize,
        // so drawString with a background colour paints an identical bounding box
        // every time and completely overwrites the previous character.
        uint16_t col = C_WHITE;
        if (gear == "R")      col = C_RED;
        else if (gear == "N") col = C_LIGHT_GREY;

        _tft.setTextColor(col, C_BG);
        _tft.setTextDatum(MC_DATUM); _tft.setTextSize(14);
        _tft.drawString(gear, CP_X + CP_W/2, MAIN_Y + MAIN_H/2 + 14);
        _tft.setTextSize(1); _tft.setTextDatum(TL_DATUM);
        _cGear = gear;
        _cSpeed = -1;
    }

    // ============================================================
    //  Tyre boxes — right panel
    //  FL(top-left) FR(top-right) RL(btm-left) RR(btm-right)
    //  Box BG color = nothing, border = temp color, text = wear%
    //  Compound badge across the top of the panel
    // ============================================================
    void _updateTyres(float tFL, float tFR, float tRL, float tRR,
                      int wFL, int wFR, int wRL, int wRR,
                      const String& compound) {
        const float t[4] = {tFL, tFR, tRL, tRR};
        const int   w[4] = {wFL, wFR, wRL, wRR};

        // Compound badge at top of right panel
        if (compound != _cComp) {
            _tft.fillRect(RP_X, MAIN_Y, RP_W, 14, C_BG);
            uint16_t cc = compoundColor(compound);
            _tft.fillRect(RP_X+2, MAIN_Y+1, 12, 12, cc);     // colored square
            _tft.setTextColor(C_WHITE, C_BG);
            _tft.setTextSize(1);
            _tft.setCursor(RP_X+17, MAIN_Y+3);
            _tft.print(compound);
            // Wear labels right-aligned
            _tft.setTextColor(C_LIGHT_GREY, C_BG);
            _tft.setCursor(RP_X+75, MAIN_Y+3);
            _tft.print("WEAR%");
            _cComp = compound;
        }

        // Box positions:
        //   FL: (RP_X+2, MAIN_Y+66)   FR: (RP_X+77, MAIN_Y+66)  W=69 H=42
        //   RL: (RP_X+2, MAIN_Y+115)  RR: (RP_X+77, MAIN_Y+115)
        // Verified: MAIN_Y+66+42=156 < 248, MAIN_Y+115+42=205 < 248 ✓
        static const int BX[4] = {RP_X+2, RP_X+77, RP_X+2, RP_X+77};
        static const int BY[4] = {MAIN_Y+66, MAIN_Y+66, MAIN_Y+115, MAIN_Y+115};
        static constexpr int BW = 69, BH = 42;

        for (int i = 0; i < 4; i++) {
            if (t[i] == _cTyreT[i] && w[i] == _cTyreW[i]) continue;

            uint16_t tc = tyreTempColor(t[i]);
            uint16_t wc = (w[i] > 75) ? C_GREEN  :
                          (w[i] > 50) ? C_YELLOW :
                          (w[i] > 25) ? C_ORANGE : C_RED;

            _tft.fillRect(BX[i], BY[i], BW, BH, C_DARK_GREY);
            _tft.drawRect(BX[i], BY[i], BW, BH, tc); // temp-colored border

            // Temperature top-left (small, temp-colored)
            char tbuf[7]; snprintf(tbuf, sizeof(tbuf), "%.0f\xB0", t[i]);
            _tft.setTextColor(tc, C_DARK_GREY);
            _tft.setTextSize(1); _tft.setTextDatum(TL_DATUM);
            _tft.drawString(tbuf, BX[i]+3, BY[i]+4);

            // Wear % center-bottom (medium, wear-colored)
            char wbuf[5]; snprintf(wbuf, sizeof(wbuf), "%d%%", constrain(w[i],0,100));
            _tft.setTextColor(wc, C_DARK_GREY);
            _tft.setTextDatum(MC_DATUM); _tft.setTextSize(2);
            _tft.drawString(wbuf, BX[i]+BW/2, BY[i]+BH/2+7);
            _tft.setTextDatum(TL_DATUM); _tft.setTextSize(1);

            _cTyreT[i] = t[i]; _cTyreW[i] = w[i];
        }
    }

    // ============================================================
    //  TC / ABS — info bar cells
    //  Active = bright color (yellow/blue), inactive = dim grey
    // ============================================================
    void _updateAids(int tc, bool tca, int abs_, bool absa) {
        if (tc == _cTC && tca == _cTCA && abs_ == _cABS && absa == _cABSA) return;
        char buf[4];
        // TC cell
        _tft.fillRect(IB_TC_X+14, IB_Y, IB_TC_W-14, IB_H, C_BG);
        _tft.setTextColor(tca ? C_YELLOW : C_MID_GREY, C_BG);
        _tft.setTextDatum(MC_DATUM); _tft.setTextSize(2);
        snprintf(buf, sizeof(buf), "%d", tc);
        _tft.drawString(buf, IB_TC_X + 14 + (IB_TC_W-14)/2, IB_Y + IB_H/2);
        // ABS cell
        _tft.fillRect(IB_ABS_X+14, IB_Y, IB_ABS_W-14, IB_H, C_BG);
        _tft.setTextColor(absa ? C_DODGER_BLUE : C_MID_GREY, C_BG);
        snprintf(buf, sizeof(buf), "%d", abs_);
        _tft.drawString(buf, IB_ABS_X + 14 + (IB_ABS_W-14)/2, IB_Y + IB_H/2);
        _tft.setTextSize(1); _tft.setTextDatum(TL_DATUM);
        _cTC = tc; _cTCA = tca; _cABS = abs_; _cABSA = absa;
    }

    // ============================================================
    //  Brake Bias / Diff — right panel bottom row
    // ============================================================
    void _updateSetup(float bb, int diff) {
        int y = MAIN_Y + 178;
        if (bb != _cBB) {
            _tft.fillRect(RP_X+2,  y, 70, 14, C_BG);
            _tft.setTextColor(C_WHITE, C_BG);
            _tft.setTextDatum(TL_DATUM); _tft.setTextSize(2);
            char buf[6]; snprintf(buf, sizeof(buf), "%.0f", bb);
            _tft.drawString(buf, RP_X+2, y);
            _cBB = bb;
        }
        if (diff != _cDiff) {
            _tft.fillRect(RP_X+77, y, 70, 14, C_BG);
            _tft.setTextColor(C_WHITE, C_BG);
            _tft.setTextDatum(TL_DATUM); _tft.setTextSize(2);
            char buf[6]; snprintf(buf, sizeof(buf), "%d%%", diff);
            _tft.drawString(buf, RP_X+77, y);
            _cDiff = diff;
        }
        _tft.setTextSize(1); _tft.setTextDatum(TL_DATUM);
    }

    // ============================================================
    //  Sector times — left panel stacked rows
    //  Purple=SB  Green=PB  Yellow=slower  Grey=no data
    // ============================================================
    void _updateSectors(const String& s1, const String& s2, const String& s3,
                        int f1, int f2, int f3) {
        bool chg[3] = {
            s1!=_cS1||f1!=_cF1,
            s2!=_cS2||f2!=_cF2,
            s3!=_cS3||f3!=_cF3
        };
        const String* times[3] = {&s1, &s2, &s3};
        const int     flags[3] = {f1, f2, f3};
        const char*   labels[3] = {"S1", "S2", "S3"};

        for (int i = 0; i < 3; i++) {
            if (!chg[i]) continue;
            int y = LP_SEC_Y + i * LP_SEC_ROW_H;
            bool isSB = (flags[i] == 3);
            uint16_t col = sectorColor(flags[i]);
            uint16_t bg = isSB ? C_PURPLE : C_BG;
            uint16_t fg = isSB ? C_WHITE : col;

            _tft.fillRect(LP_X, y, LP_W, LP_SEC_ROW_H, bg);
            _tft.setTextColor(fg, bg);
            _tft.setTextDatum(TL_DATUM); _tft.setTextSize(1);
            _tft.drawString(labels[i], LP_X+2, y+2);
            _tft.setTextDatum(TR_DATUM);
            _tft.drawString(*times[i], LP_X+LP_W-2, y+2);
            _tft.setTextDatum(TL_DATUM);
        }
        _cS1=s1; _cF1=f1; _cS2=s2; _cF2=f2; _cS3=s3; _cF3=f3;
    }

    // ============================================================
    //  ERS battery bar — full-width strip above trace
    //  Green >50%   Yellow 25-50%   Red <25%
    // ============================================================
    void _updateErsBar(int pct) {
        if (pct == _cErsPct) return;
        pct = constrain(pct, 0, 100);
        int fill = (pct * SCR_W) / 100;
        uint16_t col = (pct < 25) ? C_RED :
                       (pct < 50 ? C_YELLOW : C_GREEN);
        _tft.fillRect(0, BOT_ERS_Y, SCR_W, BOT_ERS_H, C_DARK_GREY);
        if (fill > 0) _tft.fillRect(0, BOT_ERS_Y, fill, BOT_ERS_H, col);
        _tft.drawRect(0, BOT_ERS_Y, SCR_W, BOT_ERS_H, C_MID_GREY);
        // ERS % right-aligned
        char buf[6]; snprintf(buf, sizeof(buf), "%d%%", pct);
        _tft.setTextColor(C_WHITE, C_MID_GREY);
        _tft.setTextDatum(MR_DATUM); _tft.setTextSize(1);
        _tft.drawString(buf, SCR_W-2, BOT_ERS_Y + BOT_ERS_H/2);
        _tft.setTextDatum(TL_DATUM);
        _cErsPct = pct;
    }

    // ============================================================
    //  Throttle / Brake trace — scrolling line graph, bottom strip
    //  Green = throttle  Red = brake
    // ============================================================
    void _updateTrace(int thr, int brk) {
        thr = constrain(thr, 0, 100);
        brk = constrain(brk, 0, 100);
        _thrBuf[_trHead] = (uint8_t)thr;
        _brkBuf[_trHead] = (uint8_t)brk;
        _trHead = (_trHead + 1) % TRACE_W;

        _traceSprite->fillSprite(C_BG);
        // 50% grid line
        int gy = TRACE_H - (50 * TRACE_H) / 100;
        _traceSprite->drawFastHLine(0, gy, TRACE_W, 0x1082);

        static constexpr int TRACE_WINDOW = TRACE_W / 2;
        int base = _trHead - TRACE_WINDOW;
        while (base < 0) base += TRACE_W;

        for (int col = 0; col < TRACE_W; col++) {
            int sample = (col * (TRACE_WINDOW - 1)) / (TRACE_W - 1);
            int i  = base + sample;
            if (i >= TRACE_W) i -= TRACE_W;
            int ty = TRACE_H - 1 - (_thrBuf[i] * (TRACE_H-1)) / 100;
            int by2 = TRACE_H - 1 - (_brkBuf[i] * (TRACE_H-1)) / 100;

            if (col == 0) {
                _traceSprite->drawPixel(0, ty,  C_GREEN);
                _traceSprite->drawPixel(0, by2, C_RED);
            } else {
                int prevSample = ((col - 1) * (TRACE_WINDOW - 1)) / (TRACE_W - 1);
                int pi = base + prevSample;
                if (pi >= TRACE_W) pi -= TRACE_W;
                int pty = TRACE_H - 1 - (_thrBuf[pi] * (TRACE_H-1)) / 100;
                int pby = TRACE_H - 1 - (_brkBuf[pi] * (TRACE_H-1)) / 100;
                int mn, mx;
                mn = ty < pty  ? ty  : pty;  mx = ty > pty  ? ty  : pty;
                _traceSprite->drawFastVLine(col, mn, mx-mn+1, C_GREEN);
                mn = by2 < pby ? by2 : pby;  mx = by2 > pby ? by2 : pby;
                _traceSprite->drawFastVLine(col, mn, mx-mn+1, C_RED);
            }
        }
        _traceSprite->pushSprite(TRACE_X, TRACE_Y);
    }

    // ============================================================
    //  Utility: one-line text label
    // ============================================================
    void _label(const char* txt, int x, int y, uint16_t col) {
        _tft.setTextColor(col, C_BG);
        _tft.setTextDatum(TL_DATUM); _tft.setTextSize(1);
        _tft.drawString(txt, x, y);
    }
};
