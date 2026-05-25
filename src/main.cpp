/*
 * ============================================================
 *  ESP32 WROOM Racing Dashboard
 *  Display: ILI9488 480×320 SPI
 *
 *  Wiring (adjust in platformio.ini build_flags if different):
 *    MOSI → GPIO 23
 *    MISO → GPIO 19
 *    SCK  → GPIO 18
 *    CS   → GPIO 5
 *    DC   → GPIO 2
 *    RST  → GPIO 4
 *    BL   → GPIO 15  (connect through 33Ω resistor or direct)
 *    VCC  → 3.3V
 *    GND  → GND
 *
 *  SimHub setup:
 *    - Enable "Custom Serial Device" in SimHub
 *    - Paste the contents of SHCustomProtocol.txt into the
 *      "Update messages" formula field
 *    - Set baud rate to 115200
 *    - Enable "Send data only when values change" for efficiency
 *
 *  On first data reception the dashboard activates immediately.
 * ============================================================
 */

#include <Arduino.h>
#include <TFT_eSPI.h>

#include "dashboard.h"
#include "simhub_protocol.h"
#include "dash_renderer.h"

// ---- Backlight PWM ----
static constexpr int  BL_PIN     = 15;
static constexpr int  BL_CHANNEL = 0;
static constexpr int  BL_FREQ    = 5000;
static constexpr int  BL_RES     = 8;    // 8-bit (0-255)
static constexpr int  BL_BRIGHT  = 230;  // 0-255

// ---- Global objects ----
TFT_eSPI       tft;
DashRenderer   dash(tft);
SimHubParser   parser;

// ---- Watchdog: dim screen if no data for 10 seconds ----
static constexpr unsigned long NO_DATA_DIM_MS = 10000UL;
static unsigned long lastDataMs = 0;
static bool          dimmed     = false;

void setup() {
    // Backlight — on before display init so there's no dark flash
    ledcSetup(BL_CHANNEL, BL_FREQ, BL_RES);
    ledcAttachPin(BL_PIN, BL_CHANNEL);
    ledcWrite(BL_CHANNEL, BL_BRIGHT);

    Serial.begin(115200);

    dash.begin();  // init TFT + draw chrome + idle message
}

void loop() {
    // ---- Read serial and feed parser ----
    while (Serial.available()) {
        int c = Serial.read();
        if (parser.feed(c)) {
            // Complete valid packet received
            dash.update(parser.data);
            lastDataMs = millis();

            // Restore brightness if it was dimmed
            if (dimmed) {
                ledcWrite(BL_CHANNEL, BL_BRIGHT);
                dimmed = false;
            }
        }
    }

    // ---- Dim backlight when no SimHub data ----
    if (!dimmed && (millis() - lastDataMs > NO_DATA_DIM_MS) && lastDataMs > 0) {
        ledcWrite(BL_CHANNEL, 40);  // dim to ~16%
        dimmed = true;
    }
}
