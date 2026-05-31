#pragma once
#include <Arduino.h>

// ============================================================
//  SimHub data packet — matches the SHCustomProtocol.txt formula
//  Format: "SH;" + field1 + ";" + field2 + ";" ... + "\n"
// ============================================================
struct SimHubData {
    // Basic telemetry
    String gear        = "-";
    int    speed       = 0;
    int    rpmPct      = 0;
    int    rpmRedline  = 90;

    // Lap times (formatted "M:SS.mmm")
    String curLap      = "--:--.---";
    String lastLap     = "--:--.---";
    String bestLap     = "--:--.---";
    String delta       = "+0.000";

    // Session info
    int    lap         = 0;
    int    position    = 0;
    int    numCars     = 0;
    float  fuel        = 0.0f;
    bool   lapInvalid  = false;

    // Driver aids
    int    tcLevel     = 0;
    bool   tcActive    = false;
    int    absLevel    = 0;
    bool   absActive   = false;
    float  brakeBias   = 55.0f;

    // Tyre pressures (PSI)
    float  tyrePFL     = 0.0f;
    float  tyrePFR     = 0.0f;
    float  tyrePRL     = 0.0f;
    float  tyrePRR     = 0.0f;

    // Tyre temperatures (°C)
    float  tyreTFL     = 0.0f;
    float  tyreTFR     = 0.0f;
    float  tyreTRL     = 0.0f;
    float  tyreTRR     = 0.0f;

    // DRS: 0=not in zone  1=available  2=active  3=FIA disabled
    int    drsStatus   = 0;

    // Time gaps to adjacent cars (seconds; 0 = no car / lapped)
    float  gapFront    = 0.0f;
    float  gapBehind   = 0.0f;

    // ERS
    int    ersPct      = 0;    // 0-100 %
    int    ersMode     = 0;    // 0=none  1=medium  2=hotlap  3=overtake
    String lastLapDelta = "---";
    String tyreCompound = "?";   // "SOFT" / "MED" / "HARD" / "INT" / "WET"
    int    tyreWearFL   = 100;   // % remaining (100 = new, 0 = dead)
    int    tyreWearFR   = 100;
    int    tyreWearRL   = 100;
    int    tyreWearRR   = 100;

    // Sector times (last completed lap) + color flags
    // Flag: 3=purple/session-best  2=green/PB  1=yellow/slower  0=no data
    String s1Time  = "-.---";
    String s2Time  = "-.---";
    String s3Time  = "-.---";
    int    s1Flag  = 0;
    int    s2Flag  = 0;
    int    s3Flag  = 0;
    int    throttle    = 0;
    int    brake       = 0;

    // F1 car setup — differential (0–100 %)
    int    diffOnThrottle  = 50;
    int    diffOffThrottle = 50;

    // Safety car / VSC / pit lane limiter
    int   safetyCarStatus  = 0;    // 0=none  1=safety car  2=virtual safety car
    float scRequiredDelta  = 0.0f; // target gap to maintain behind SC
    bool  pitLimiterActive = false;
    int   pitSpeedLimit    = 80;   // km/h

    bool valid = false;
};

// ============================================================
//  Parser
// ============================================================
class SimHubParser {
public:
    bool feed(int c) {
        if (c < 0) return false;

        if (!_synced) {
            _syncBuf[_syncIdx++ % 3] = (char)c;
            int si = _syncIdx % 3;
            char s0 = _syncBuf[(si + 0) % 3];
            char s1 = _syncBuf[(si + 1) % 3];
            char s2 = _syncBuf[(si + 2) % 3];
            if (s0 == 'S' && s1 == 'H' && s2 == ';') {
                _synced = true;
                _line = "";
                _syncIdx = 0;
            }
            return false;
        }

        if ((char)c == '\n') {
            bool ok = _parse(_line);
            _line = "";
            _synced = false;
            _syncIdx = 0;
            return ok;
        }
        if (_line.length() < 512) {
            _line += (char)c;
        }
        return false;
    }

    SimHubData data;

private:
    bool   _synced   = false;
    char   _syncBuf[3] = {0, 0, 0};
    int    _syncIdx  = 0;
    String _line;

    String field(const String& s, int idx) {
        int start = 0, found = 0;
        for (int i = 0; i <= (int)s.length(); i++) {
            if (i == (int)s.length() || s[i] == ';') {
                if (found == idx) return s.substring(start, i);
                start = i + 1;
                found++;
            }
        }
        return "";
    }

    String _normalizeCompound(const String& raw) {
        String c = raw;
        c.trim();
        if (c.isEmpty()) return "?";
        c.toUpperCase();

        if (c == "MEDIUM") return "MED";
        if (c == "INTERMEDIATE" || c == "INTERMEDIATES" || c == "INTER" || c == "INTERS") return "INT";
        if (c == "WETS") return "WET";
        if (c == "SOFTS") return "SOFT";
        if (c == "HARDS") return "HARD";
        if (c == "S") return "SOFT";
        if (c == "M") return "MED";
        if (c == "H") return "HARD";
        if (c == "I") return "INT";
        if (c == "W") return "WET";
        if (c == "C3") return "SOFT";
        if (c == "C4") return "MED";
        if (c == "C5") return "HARD";
        if (c == "W") return "WET";
        if (c == "I") return "INT";
        if (c == "W") return "WET";
        if (c == "NONE" || c == "UNKNOWN" || c == "N/A" || c == "NA") return "?";
        if (c == "SOFT" || c == "MED" || c == "HARD" || c == "INT" || c == "WET") return c;

        bool numeric = true;
        for (unsigned int j = 0; j < c.length(); j++) {
            char ch = c[j];
            if (ch < '0' || ch > '9') { numeric = false; break; }
        }
        if (numeric) {
            int code = c.toInt();
            switch (code) {
                case 1:  return "SOFT";
                case 2:  return "MED";
                case 3:  return "HARD";
                case 4:  return "INT";
                case 5:  return "WET";
                case 7:  return "INT";
                case 8:  return "WET";
                case 16: return "SOFT";
                case 17: return "MED";
                case 18: return "HARD";
                case 19: return "HARD";
                case 20: return "HARD";
                default: return "?";
            }
        }

        return c;
    }

    bool _parse(const String& line) {
        if (line.length() < 5) return false;

        SimHubData next = data;
        int i = 0;
        String f;

        f = field(line, i++);  if (!f.isEmpty()) next.gear = f;
        if (next.gear.isEmpty()) next.gear = "-";

        f = field(line, i++);  if (!f.isEmpty()) next.speed = f.toInt();
        f = field(line, i++);  if (!f.isEmpty()) next.rpmPct = f.toInt();
        f = field(line, i++);  if (!f.isEmpty()) next.rpmRedline = f.toInt();

        f = field(line, i++);  if (!f.isEmpty()) next.curLap = f;
        f = field(line, i++);  if (!f.isEmpty()) next.lastLap = f;
        f = field(line, i++);  if (!f.isEmpty()) next.bestLap = f;
        f = field(line, i++);  if (!f.isEmpty()) next.delta = f;

        f = field(line, i++);  if (!f.isEmpty()) next.lap = f.toInt();
        f = field(line, i++);  if (!f.isEmpty()) next.position = f.toInt();
        f = field(line, i++);  if (!f.isEmpty()) next.numCars = f.toInt();
        f = field(line, i++);  if (!f.isEmpty()) next.fuel = f.toFloat();

        f = field(line, i++);  if (!f.isEmpty()) next.lapInvalid = (f == "True" || f == "1");
        f = field(line, i++);  if (!f.isEmpty()) next.tcLevel = f.toInt();
        f = field(line, i++);  if (!f.isEmpty()) next.tcActive = (f == "1" || f == "True");
        f = field(line, i++);  if (!f.isEmpty()) next.absLevel = f.toInt();
        f = field(line, i++);  if (!f.isEmpty()) next.absActive = (f == "1" || f == "True");
        f = field(line, i++);  if (!f.isEmpty()) next.brakeBias = f.toFloat();

        f = field(line, i++);  if (!f.isEmpty()) next.tyrePFL = f.toFloat();
        f = field(line, i++);  if (!f.isEmpty()) next.tyrePFR = f.toFloat();
        f = field(line, i++);  if (!f.isEmpty()) next.tyrePRL = f.toFloat();
        f = field(line, i++);  if (!f.isEmpty()) next.tyrePRR = f.toFloat();
        f = field(line, i++);  if (!f.isEmpty()) next.tyreTFL = f.toFloat();
        f = field(line, i++);  if (!f.isEmpty()) next.tyreTFR = f.toFloat();
        f = field(line, i++);  if (!f.isEmpty()) next.tyreTRL = f.toFloat();
        f = field(line, i++);  if (!f.isEmpty()) next.tyreTRR = f.toFloat();

        f = field(line, i++);  if (!f.isEmpty()) next.throttle = f.toInt();
        f = field(line, i++);  if (!f.isEmpty()) next.brake = f.toInt();
        f = field(line, i++);  if (!f.isEmpty()) next.diffOnThrottle = f.toInt();
        f = field(line, i++);  if (!f.isEmpty()) next.diffOffThrottle = f.toInt();

        String s1 = field(line, i++);
        String s2 = field(line, i++);
        String s3 = field(line, i++);
        if (!s1.isEmpty()) next.s1Time = s1;
        if (!s2.isEmpty()) next.s2Time = s2;
        if (!s3.isEmpty()) next.s3Time = s3;
        f = field(line, i++);  if (!f.isEmpty()) next.s1Flag = f.toInt();
        f = field(line, i++);  if (!f.isEmpty()) next.s2Flag = f.toInt();
        f = field(line, i++);  if (!f.isEmpty()) next.s3Flag = f.toInt();

        if (next.s1Flag == 0 && (next.s1Time == "0.000" || next.s1Time == "0")) next.s1Time = "-.---";
        if (next.s2Flag == 0 && (next.s2Time == "0.000" || next.s2Time == "0")) next.s2Time = "-.---";
        if (next.s3Flag == 0 && (next.s3Time == "0.000" || next.s3Time == "0")) next.s3Time = "-.---";
        if (next.s1Time.isEmpty()) next.s1Time = "-.---";
        if (next.s2Time.isEmpty()) next.s2Time = "-.---";
        if (next.s3Time.isEmpty()) next.s3Time = "-.---";

        f = field(line, i++);  if (!f.isEmpty()) next.drsStatus = f.toInt();
        f = field(line, i++);  if (!f.isEmpty()) next.gapFront = f.toFloat();
        f = field(line, i++);  if (!f.isEmpty()) next.gapBehind = f.toFloat();
        f = field(line, i++);  if (!f.isEmpty()) {
            float ersVal = f.toFloat();
            if (ersVal > 1000.0f) ersVal = ersVal / 40000.0f;
            if (ersVal < 0.0f) ersVal = 0.0f;
            if (ersVal > 100.0f) ersVal = 100.0f;
            next.ersPct = (int)(ersVal + 0.5f);
        }
        f = field(line, i++);  if (!f.isEmpty()) next.ersMode = f.toInt();

        f = field(line, i++);  if (!f.isEmpty()) next.lastLapDelta = f;
        if (next.lastLapDelta.isEmpty()) next.lastLapDelta = "---";
        f = field(line, i++);  if (!f.isEmpty()) next.tyreCompound = f;
        String normComp = _normalizeCompound(next.tyreCompound);
        if (normComp == "?" && !f.isEmpty() && !data.tyreCompound.isEmpty() && data.tyreCompound != "?") {
            normComp = data.tyreCompound;
        }
        next.tyreCompound = normComp;
        f = field(line, i++);  if (!f.isEmpty()) next.tyreWearFL = f.toInt();
        f = field(line, i++);  if (!f.isEmpty()) next.tyreWearFR = f.toInt();
        f = field(line, i++);  if (!f.isEmpty()) next.tyreWearRL = f.toInt();
        f = field(line, i++);  if (!f.isEmpty()) next.tyreWearRR = f.toInt();

        // Appended fields — only overwrite when present
        f = field(line, i++);  if (!f.isEmpty()) next.safetyCarStatus = f.toInt();
        f = field(line, i++);  if (!f.isEmpty()) next.scRequiredDelta = f.toFloat();
        f = field(line, i++);  if (!f.isEmpty()) next.pitLimiterActive = (f == "1" || f == "True");
        f = field(line, i++);  if (!f.isEmpty()) { int psl = f.toInt(); if (psl > 0) next.pitSpeedLimit = psl; }

        auto zeroLikeTime = [](const String& s) -> bool {
            return s.isEmpty() || s == "--:--.---" || s == "-.---" ||
                   s == "0" || s == "0.0" || s == "0.00" || s == "0.000" ||
                   s == "00:00.000" || s == "0:00.000";
        };

        bool active = false;
        if (next.speed > 0 || next.rpmPct > 0 || next.lap > 0 || next.position > 0 || next.numCars > 0) active = true;
        if (next.throttle > 0 || next.brake > 0 || next.ersPct > 0) active = true;
        if (next.gapFront > 0.001f || next.gapBehind > 0.001f || next.scRequiredDelta > 0.001f) active = true;
        if (next.drsStatus > 0 || next.safetyCarStatus > 0 || next.pitLimiterActive) active = true;
        if (next.s1Flag > 0 || next.s2Flag > 0 || next.s3Flag > 0) active = true;
        if (!zeroLikeTime(next.curLap) || !zeroLikeTime(next.lastLap) || !zeroLikeTime(next.bestLap)) active = true;
        if (next.delta != "+0.000" && next.delta != "-0.000") active = true;
        if (next.tyreCompound != "?" || next.tyreWearFL != 100 || next.tyreWearFR != 100 ||
            next.tyreWearRL != 100 || next.tyreWearRR != 100) active = true;

        if (!active) return false;

        next.valid = true;
        data = next;
        return true;
    }
};
