/*
    LE Gear Detector - XIAO ESP32-C5

    Passive 5GHz/2.4GHz WiFi + BLE scanner that watches for OUIs and SSIDs on
    a law-enforcement-equipment watchlist and signals hits with a flashing
    screen border + buzzer, faster as the signal gets stronger.

    Three-band phase machine: the C5 has one radio behind a dual-band
    diplexer, so only one of WiFi-5GHz / WiFi-2.4GHz / BLE is ever up at a
    time. Reworked 2026-10-01 from a flat 5GHz->2.4GHz->BLE->repeat round
    robin to a 5GHz-dominant schedule, based on real drive data and a
    measured radio-switch cost (see config.h) - 5GHz sweeps continuously,
    with a short BLE dip and a rare 2.4GHz token check interleaved every few
    sweeps rather than every cycle. See config.h for the reasoning and the
    tunable constants.

    No physical buttons in this design (config lives on the SD card, not yet
    built - see CLAUDE.md) - the schedule always runs on its own, no manual
    override.
*/

#include <Arduino.h>
#include "config.h"
#include "detector.h"
#include "alert.h"
#include "ui.h"

enum Phase : uint8_t { PHASE_WIFI_5G, PHASE_WIFI_2G, PHASE_BLE };

static Phase    s_phase          = PHASE_WIFI_5G;
static uint32_t s_phaseStart     = 0;
static uint32_t s_lastUi         = 0;
static uint8_t  s_sweepsSinceBle = 0;
static uint8_t  s_sweepsSince2G  = 0;

static const char* phaseLabel(Phase p) {
    switch (p) {
        case PHASE_WIFI_5G: return "5GHz WiFi";
        case PHASE_WIFI_2G: return "2.4GHz WiFi";
        case PHASE_BLE:     return "BLE";
    }
    return "?";
}

// Stopping both unconditionally is safe - each stop function is a no-op if
// that stack isn't the one currently up.
static void switchToPhase(Phase next) {
    detectorStopWifiPhase();
    detectorStopBlePhase();

    switch (next) {
        case PHASE_WIFI_5G: detectorStartWifi5GPhase(); break;
        case PHASE_WIFI_2G: detectorStartWifi2GPhase(); break;
        case PHASE_BLE:     detectorStartBlePhase();    break;
    }

    s_phase      = next;
    s_phaseStart = millis();
}

// Decide what follows a completed 5GHz sweep: a rare 2.4GHz token check, an
// occasional short BLE dip, or straight into another 5GHz sweep (the common
// case - 5GHz is the dominant phase by design).
static Phase pickAfter5G() {
    s_sweepsSinceBle++;
    s_sweepsSince2G++;

    if (s_sweepsSince2G >= WIFI_2G_EVERY_N_SWEEPS) {
        s_sweepsSince2G = 0;
        return PHASE_WIFI_2G;
    }
    if (s_sweepsSinceBle >= BLE_EVERY_N_SWEEPS) {
        s_sweepsSinceBle = 0;
        return PHASE_BLE;
    }
    return PHASE_WIFI_5G;
}

void setup() {
    Serial.begin(115200);

    uiInit();
    uiBootScreen();

    alertInit();
    detectorInit();

    switchToPhase(PHASE_WIFI_5G);
}

void loop() {
    uint32_t now = millis();

    uint32_t phaseDuration;
    switch (s_phase) {
        case PHASE_WIFI_5G: detectorHopChannel(); phaseDuration = WIFI_5G_PHASE_MS; break;
        case PHASE_WIFI_2G: detectorHopChannel(); phaseDuration = WIFI_2G_PHASE_MS; break;
        default:             /* PHASE_BLE */       phaseDuration = BLE_PHASE_MS;     break;
    }

    if (now - s_phaseStart >= phaseDuration) {
        if (s_phase == PHASE_WIFI_5G) {
            Phase next = pickAfter5G();
            if (next == PHASE_WIFI_5G) {
                // Staying in 5GHz for another sweep - the radio's already
                // up and tuned, just reset the phase timer. Going through
                // switchToPhase() here would tear down and reinit the WiFi
                // stack for no reason every single sweep.
                s_phaseStart = now;
            } else {
                switchToPhase(next);
            }
        } else {
            switchToPhase(PHASE_WIFI_5G);
        }
    }

    detectorExpire();

    DetectorStatus st = detectorStatus();
    alertUpdate(st, ALERT_HOLD_MS);

    // Display is comparatively slow, so throttle it.
    if (now - s_lastUi >= UI_REFRESH_MS) {
        uiRender(st, phaseLabel(s_phase));
        s_lastUi = now;
    }
}
