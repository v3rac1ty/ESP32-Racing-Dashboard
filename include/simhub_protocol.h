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

    // Inputs (0-100)
    int    throttle    = 0;
    int    brake       = 0;

    bool valid = false;   // true once at least one packet received
};

// ============================================================
//  Parser
// ============================================================
class SimHubParser {
public:
    // Feed bytes into the parser; returns true when a complete packet is parsed.
    bool feed(int c) {
        if (c < 0) return false;

        // Look for "SH;" sync header
        if (!_synced) {
            _syncBuf[_syncIdx++ % 3] = (char)c;
            // Check last 3 chars
            char a = _syncBuf[0], b = _syncBuf[1], cc = _syncBuf[2];
            // Circular: need to check order carefully
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

        // Accumulate until newline
        if ((char)c == '\n') {
            bool ok = _parse(_line);
            _line = "";
            _synced = false;  // require re-sync for next packet
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

    // Split by ';', return field at index
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

    bool _parse(const String& line) {
        if (line.length() < 5) return false;

        // Field order matches SHCustomProtocol.txt exactly
        int i = 0;
        data.gear       = field(line, i++);  if (data.gear.isEmpty()) data.gear = "-";
        data.speed      = field(line, i++).toInt();
        data.rpmPct     = field(line, i++).toInt();
        data.rpmRedline = field(line, i++).toInt();
        data.curLap     = field(line, i++);
        data.lastLap    = field(line, i++);
        data.bestLap    = field(line, i++);
        data.delta      = field(line, i++);
        data.lap        = field(line, i++).toInt();
        data.position   = field(line, i++).toInt();
        data.numCars    = field(line, i++).toInt();
        data.fuel       = field(line, i++).toFloat();
        String lapInvStr= field(line, i++);
        data.lapInvalid = (lapInvStr == "True" || lapInvStr == "1");
        data.tcLevel    = field(line, i++).toInt();
        String tcAct    = field(line, i++);
        data.tcActive   = (tcAct == "1" || tcAct == "True");
        data.absLevel   = field(line, i++).toInt();
        String absAct   = field(line, i++);
        data.absActive  = (absAct == "1" || absAct == "True");
        data.brakeBias  = field(line, i++).toFloat();
        data.tyrePFL    = field(line, i++).toFloat();
        data.tyrePFR    = field(line, i++).toFloat();
        data.tyrePRL    = field(line, i++).toFloat();
        data.tyrePRR    = field(line, i++).toFloat();
        data.tyreTFL    = field(line, i++).toFloat();
        data.tyreTFR    = field(line, i++).toFloat();
        data.tyreTRL    = field(line, i++).toFloat();
        data.tyreTRR    = field(line, i++).toFloat();
        data.throttle   = field(line, i++).toInt();
        data.brake      = field(line, i++).toInt();

        data.valid = true;
        return true;
    }
};
