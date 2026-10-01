// SPDX-License-Identifier: GPL-3.0-only
//
// Ported from legacy/src/ui.cpp (128x64 monochrome OLED via u8g2) to the
// ILI9341 colour display. Layout mirrors the original almost exactly -
// header state bar, auto-sized vendor name, match-detail line, RSSI bar (or
// a "no matched gear" watchlist summary) - just in colour, on a bigger
// screen, with one real change: the original signalled an alert via a
// physical LED; this design has none (decision: "No status LED"), so the
// flashing screen border (synced to the buzzer via alertOutputOn(), per
// decision #3) is the alert indicator instead. The header's ALL CLEAR / **
// ALERT ** text is otherwise unchanged from the original, non-flashing.

#include "ui.h"
#include "alert.h"
#include "config.h"
#include "oui_table.h"
#include "ssid_table.h"

#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>

static Adafruit_ILI9341 tft(PIN_TFT_CS, PIN_TFT_DC, PIN_TFT_RST);

static const uint16_t COLOR_BG      = ILI9341_BLACK;
static const uint16_t COLOR_TEXT    = ILI9341_WHITE;
static const uint16_t COLOR_ALERT   = ILI9341_RED;
static const uint8_t  BORDER_THICKNESS = 6;
static const uint8_t  MARGIN = BORDER_THICKNESS + 6;

static uint16_t screenW() { return tft.width(); }
static uint16_t screenH() { return tft.height(); }

void uiInit() {
    SPI.begin(PIN_TFT_SCK, PIN_TFT_MISO, PIN_TFT_MOSI, PIN_TFT_CS);
    tft.begin();
    tft.setRotation(1);
    tft.fillScreen(COLOR_BG);
}

void uiBootScreen() {
    tft.fillScreen(COLOR_BG);
    tft.setTextColor(COLOR_TEXT);

    tft.setTextSize(3);
    tft.setCursor(MARGIN, MARGIN);
    tft.println("LE GEAR DETECTOR");
    tft.drawFastHLine(MARGIN, MARGIN + 28, screenW() - 2 * MARGIN, COLOR_TEXT);

    char line[48];
    tft.setTextSize(2);

    tft.setCursor(MARGIN, MARGIN + 44);
    snprintf(line, sizeof(line), "OUI entries: %u", (unsigned)ouiTableSize());
    tft.println(line);

    tft.setCursor(MARGIN, MARGIN + 68);
    snprintf(line, sizeof(line), "SSID entries: %u", (unsigned)ssidTableSize());
    tft.println(line);

    tft.setCursor(MARGIN, MARGIN + 92);
    snprintf(line, sizeof(line), "BLE scan: %s", BLE_SCAN_PASSIVE ? "passive" : "ACTIVE");
    tft.println(line);

    tft.setCursor(MARGIN, MARGIN + 116);
    snprintf(line, sizeof(line), "WiFi 5GHz: %u ch, 2.4GHz: %u ch (token)",
             (unsigned)WIFI_5G_CHANNEL_COUNT, (unsigned)WIFI_2G_CHANNEL_COUNT);
    tft.println(line);

    tft.setCursor(MARGIN, MARGIN + 140);
    tft.println("Starting...");

    delay(800);
    tft.fillScreen(COLOR_BG);
}

static void drawBorder(bool on) {
    uint16_t color = on ? COLOR_ALERT : COLOR_BG;
    for (uint8_t i = 0; i < BORDER_THICKNESS; i++) {
        tft.drawRect(i, i, screenW() - 2 * i, screenH() - 2 * i, color);
    }
}

// Largest text size (Adafruit_GFX's integer font-size multiplier) that keeps
// `text` within maxWidth, down to 1 as a guaranteed-fit fallback. Mirrors
// the original's "biggest font that fits, else drop a tier" vendor-name
// sizing - same idea, GFX's coarser size steps instead of u8g2's font list.
static uint8_t fitTextSize(const char* text, uint8_t maxSize, uint16_t maxWidth) {
    for (uint8_t size = maxSize; size >= 1; size--) {
        tft.setTextSize(size);
        int16_t x1, y1;
        uint16_t w, h;
        tft.getTextBounds(text, 0, 0, &x1, &y1, &w, &h);
        if (w <= maxWidth) return size;
    }
    return 1;
}

static const uint8_t HEADER_H = 32;

// Header and content are redrawn independently, each only when its own
// inputs change. Phase label ticks over every few seconds (much more often
// than the match state changes) - redrawing the whole screen for that alone
// was the visible "flicker on phase change" this splits apart.
static bool      s_headerEverRendered = false;
static char      s_lastPhase[16]      = "";
static AlertState s_lastAlertState    = ALERT_CLEAR;

static bool            s_contentEverRendered = false;
static uint8_t          s_lastCount    = 0xFF;
static int16_t          s_lastRssi     = -127;
static const TrackedDevice* s_lastBest = nullptr;
static const int16_t    RSSI_REDRAW_DEADBAND = 3;

static void drawHeader(bool alerting, const char* phaseLabel) {
    tft.fillRect(MARGIN - 4, MARGIN - 4, screenW() - 2 * (MARGIN - 4), HEADER_H, COLOR_BG);

    if (alerting) {
        tft.fillRect(MARGIN - 4, MARGIN - 4, screenW() - 2 * (MARGIN - 4), HEADER_H, COLOR_ALERT);
        tft.setTextColor(COLOR_TEXT);
        tft.setTextSize(2);
        tft.setCursor(MARGIN + 4, MARGIN + 4);
        tft.print("** ALERT **");
    } else {
        tft.setTextColor(COLOR_TEXT);
        tft.setTextSize(2);
        tft.setCursor(MARGIN + 4, MARGIN + 4);
        tft.print("ALL CLEAR");
        tft.drawFastHLine(MARGIN - 4, MARGIN - 4 + HEADER_H,
                           screenW() - 2 * (MARGIN - 4), COLOR_TEXT);
    }
    // Phase indicator sits on the header bar at a fixed x, same reasoning as
    // the original: a shorter/longer label (e.g. "5GHz WiFi" vs "BLE")
    // shouldn't shift other header text around.
    tft.setTextColor(COLOR_TEXT);
    tft.setTextSize(2);
    tft.setCursor(screenW() - 150, MARGIN + 4);
    tft.print(phaseLabel);
}

static void drawContent(const DetectorStatus& st) {
    const uint16_t contentTop = MARGIN - 4 + HEADER_H + 10;
    const uint16_t contentW   = screenW() - 2 * (MARGIN - 4) - 8;
    char line[48];

    tft.fillRect(MARGIN - 4, contentTop - 6,
                 screenW() - 2 * (MARGIN - 4),
                 screenH() - (contentTop - 6) - (MARGIN - 4), COLOR_BG);

    if (st.best) {
        // --- vendor name, as large as fits ---
        uint8_t size = fitTextSize(st.best->vendor, 5, contentW);
        tft.setTextSize(size);
        tft.setTextColor(COLOR_TEXT);
        tft.setCursor(MARGIN, contentTop + 10);
        tft.print(st.best->vendor);

        // --- secondary line: what actually matched, and which radio/band -
        // useful for troubleshooting, not just "it's an OUI hit" ---
        tft.setTextSize(2);
        tft.setCursor(MARGIN, contentTop + 70);
        const char* band = (st.best->band == BAND_5G) ? "5GHz" :
                            (st.best->band == BAND_2G) ? "2.4GHz" : "";
        if (st.best->ssid[0] != '\0') {
            const char* how = (st.best->source == SRC_PROBE) ? "probe" : "beacon";
            snprintf(line, sizeof(line), "SSID %s (%s): %s", how, band, st.best->ssid);
        } else if (st.best->source == SRC_BLE) {
            snprintf(line, sizeof(line), "via BLE");
        } else {
            snprintf(line, sizeof(line), "via %s WiFi OUI", band);
        }
        tft.print(line);

        // --- signal strength bar ---
        int16_t r = st.bestRssi;
        if (r < RSSI_WEAK)   r = RSSI_WEAK;
        if (r > RSSI_STRONG) r = RSSI_STRONG;
        int barMaxW = contentW - 70;
        int barW = (int)(((int32_t)r - RSSI_WEAK) * barMaxW /
                         ((int32_t)RSSI_STRONG - RSSI_WEAK));

        int barY = contentTop + 96;
        tft.drawRect(MARGIN, barY, barMaxW, 18, COLOR_TEXT);
        if (barW > 0) tft.fillRect(MARGIN + 1, barY + 1, barW, 16, COLOR_TEXT);

        tft.setTextSize(2);
        tft.setCursor(MARGIN + barMaxW + 8, barY + 1);
        snprintf(line, sizeof(line), "%ddB", (int)st.bestRssi);
        tft.print(line);
    } else {
        tft.setTextColor(COLOR_TEXT);
        tft.setTextSize(2);

        tft.setCursor(MARGIN, contentTop + 10);
        tft.print("No matched gear");

        tft.setCursor(MARGIN, contentTop + 40);
        snprintf(line, sizeof(line), "Watchlist: %u OUI", (unsigned)ouiTableSize());
        tft.print(line);

        tft.setCursor(MARGIN, contentTop + 64);
        snprintf(line, sizeof(line), "%u SSID pattern%s", (unsigned)ssidTableSize(),
                 ssidTableSize() == 1 ? "" : "s");
        tft.print(line);
    }
}

void uiRender(const DetectorStatus& st, const char* phaseLabel) {
    drawBorder(alertOutputOn());

    AlertState alertNow = alertState();
    bool headerChanged = !s_headerEverRendered ||
                          (alertNow != s_lastAlertState) ||
                          (strcmp(phaseLabel, s_lastPhase) != 0);
    if (headerChanged) {
        s_headerEverRendered = true;
        s_lastAlertState     = alertNow;
        strncpy(s_lastPhase, phaseLabel, sizeof(s_lastPhase) - 1);
        s_lastPhase[sizeof(s_lastPhase) - 1] = '\0';
        drawHeader(alertNow == ALERT_ACTIVE, phaseLabel);
    }

    bool rssiMoved = abs((int)st.bestRssi - (int)s_lastRssi) >= RSSI_REDRAW_DEADBAND;
    bool contentChanged = !s_contentEverRendered ||
                          (st.activeCount != s_lastCount) ||
                          (st.best != s_lastBest) ||
                          rssiMoved;
    if (contentChanged) {
        s_contentEverRendered = true;
        s_lastCount = st.activeCount;
        s_lastRssi  = st.bestRssi;
        s_lastBest  = st.best;
        drawContent(st);
    }
}
