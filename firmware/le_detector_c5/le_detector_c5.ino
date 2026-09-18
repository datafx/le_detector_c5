/*
    LE Gear Detector - XIAO ESP32-C5

    Passive 5GHz/2.4GHz WiFi + BLE scanner that watches for OUIs and SSIDs on
    a law-enforcement-equipment watchlist and signals hits with a flashing
    screen border + buzzer, faster as the signal gets stronger.

    Three-band phase machine: the C5 has one radio behind a dual-band
    diplexer, so it cycles 5GHz -> 2.4GHz -> BLE -> repeat, one radio stack
    up at a time. 5GHz gets the largest time budget - it's the primary
    target (Axon Fleet Hub beacons continuously there).

    No physical buttons in this design (config lives on the SD card, not yet
    built - see CLAUDE.md) - the band cycle always runs all three phases.
*/

#include <Arduino.h>
#include "config.h"
#include "detector.h"
#include "alert.h"
#include "ui.h"

enum Phase : uint8_t { PHASE_WIFI_5G, PHASE_WIFI_2G, PHASE_BLE };

static Phase    s_phase      = PHASE_WIFI_5G;
static uint32_t s_phaseStart = 0;
static uint32_t s_lastUi     = 0;

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
    Phase    nextPhase;

    switch (s_phase) {
        case PHASE_WIFI_5G:
            detectorHopChannel();
            phaseDuration = WIFI_5G_PHASE_MS;
            nextPhase     = PHASE_WIFI_2G;
            break;
        case PHASE_WIFI_2G:
            detectorHopChannel();
            phaseDuration = WIFI_2G_PHASE_MS;
            nextPhase     = PHASE_BLE;
            break;
        default: // PHASE_BLE
            phaseDuration = BLE_PHASE_MS;
            nextPhase     = PHASE_WIFI_5G;
            break;
    }

    if (now - s_phaseStart >= phaseDuration) {
        switchToPhase(nextPhase);
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
