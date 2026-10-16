// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <Arduino.h>

// ---------------------------------------------------------------------------
// Hardware pins (decision #6 in CLAUDE.md - confirmed against physical board)
// ---------------------------------------------------------------------------
static const uint8_t PIN_TFT_CS   = 25;  // D2
static const uint8_t PIN_TFT_DC   = 7;   // D3
static const uint8_t PIN_TFT_RST  = 23;  // D4
static const uint8_t PIN_SD_CS    = 24;  // D5 (reserved, not read until SD bring-up)
static const uint8_t PIN_TFT_SCK  = 8;   // D8, shared SPI bus
static const uint8_t PIN_TFT_MISO = 9;   // D9, shared SPI bus
static const uint8_t PIN_TFT_MOSI = 10;  // D10, shared SPI bus
static const uint8_t PIN_BUZZER   = 1;   // D0

// No PIN_LED - the screen border flashes instead (decision: "No status LED").
// No PIN_BOOT - no physical buttons; config lives on the SD card (future).

// ---------------------------------------------------------------------------
// Radio scheduling - three-band phase machine
//
// The C5 has ONE radio behind a dual-band diplexer: it cannot listen to
// 2.4 and 5 GHz simultaneously, and still cannot do WiFi and BLE at once.
// So three phases, time-sliced, one radio stack up at a time.
//
// Reworked 2026-10-01 based on real drive data (Kismet, long highway drive)
// and a measured radio-switch cost on this board: WiFi->BLE = 103ms,
// BLE->WiFi = 9ms (perfectly consistent across 8 iterations, zero variance -
// see firmware/05_radio_switch_timing). Switching is cheap, so the duty
// cycle is driven by what's actually useful, not switch-cost avoidance:
//
// - 5 GHz is the only long-range, continuous signal (beacons every ~100ms,
//   seen at Axon Fleet Hub range up to ~0.5mi in the drive data) and is
//   therefore the dominant phase by a wide margin.
// - BLE is short-range (~10m) and most useful for a stopped/slow/adjacent
//   encounter, which is a multi-second situation, not a sub-second one - a
//   fast highway pass is barely catchable at any reasonable scan frequency
//   regardless, so BLE doesn't need to run every cycle. A short dip every
//   few 5GHz sweeps is enough.
// - 2.4 GHz contributes almost nothing on the road per the drive data
//   (PSP-MVR/Cradlepoint-type hits are a fixed-barracks-proximity signal,
//   not a mobile one) - kept only as a rare token check on the 3
//   non-overlapping channels (1/6/11), not a full 1-11 sweep.
// ---------------------------------------------------------------------------
static const uint8_t WIFI_5G_CHANNELS[]  = {36, 40, 44, 48, 149, 153, 157, 161, 165};
static const uint8_t WIFI_5G_CHANNEL_COUNT = sizeof(WIFI_5G_CHANNELS) / sizeof(WIFI_5G_CHANNELS[0]);
static const uint32_t WIFI_5G_PHASE_MS   = 2700;  // exactly one sweep of 9 channels at 300ms hop
static const uint32_t WIFI_5G_HOP_MS     = 300;

// Non-overlapping US channels only - a token check for stationary facility
// APs, not a mobile detection path. See note above.
static const uint8_t WIFI_2G_CHANNELS[]  = {1, 6, 11};
static const uint8_t WIFI_2G_CHANNEL_COUNT = sizeof(WIFI_2G_CHANNELS) / sizeof(WIFI_2G_CHANNELS[0]);
static const uint32_t WIFI_2G_PHASE_MS    = 750;  // one pass of 3 channels at 250ms hop
static const uint32_t WIFI_2G_HOP_MS      = 250;

static const uint32_t BLE_PHASE_MS        = 400;  // short dip, not a long window

// How many 5GHz sweeps happen between a BLE dip / a 2.4GHz token visit.
// Tune these empirically - the right split is still unknown, this is a
// starting point based on the reasoning above, not a measured optimum.
static const uint8_t BLE_EVERY_N_SWEEPS      = 3;
static const uint8_t WIFI_2G_EVERY_N_SWEEPS  = 8;

// Passive = receive only, we never transmit. Set false only if you
// specifically want active BLE scanning (locked decision: stays passive).
static const bool BLE_SCAN_PASSIVE = true;

// ---------------------------------------------------------------------------
// Device tracking
// ---------------------------------------------------------------------------
static const uint8_t  MAX_TRACKED       = 48;    // fixed table, LRU eviction
static const uint32_t DEVICE_TTL_MS     = 10000; // drop entry if unseen this long
static const uint8_t  RSSI_EMA_NUM      = 1;     // EMA smoothing: new weight
static const uint8_t  RSSI_EMA_DEN      = 3;     //   value = (new*1 + old*2)/3

// ---------------------------------------------------------------------------
// Alert behaviour
//
// Screen border and buzzer are driven from a single phase so they are always
// in sync. Flash rate scales with the strongest active signal: weak = slow,
// strong = fast.
// ---------------------------------------------------------------------------
// Pure decay timer: resets to full duration on every qualifying receive
// (any matched device, not just the one that originally tripped the alert),
// never a latch. Three-phase mode needs enough slack to survive a full
// 5GHz->2.4GHz->BLE round trip plus missed-frame margin.
static const uint32_t ALERT_HOLD_MS       = 14000;

static const int8_t   RSSI_WEAK         = -95;   // maps to the slowest flash
// -45 was an assumed ceiling, never measured. Field data: parked right next
// to real target vehicles tops out around -65 to -60 dBm (embedded, modest-
// power radios read through a vehicle body/window, not free-space range) -
// -45 was never reachable in practice, so the bar/flash rate never hit full
// scale. -65 is now "as strong as it gets" - full bar, fastest flash.
static const int8_t   RSSI_STRONG       = -65;   // maps to the fastest flash
static const uint32_t FLASH_PERIOD_SLOW = 1000;  // ms, full on+off cycle
static const uint32_t FLASH_PERIOD_FAST = 150;

// Buzzer follows the border flash exactly, but capped so a fast flash chirps
// instead of droning. Set to a large value to make it perfectly 1:1.
static const uint32_t BUZZER_MAX_ON_MS  = 70;

// Set false to run silently (bench testing without the noise).
static const bool BUZZER_ENABLED = true;

// Confirmed 2026-09-18: this board's buzzer module droned continuously at
// idle and only went silent on the brief "on" pulse - a PNP/active-low
// driver stage, same as the common cheap-module behavior. The bring-up
// step 3 test (symmetric 500ms HIGH/LOW toggle) couldn't have caught this -
// it exercises both levels equally either way, so it sounds like normal
// beeping regardless of which level is actually "on".
static const bool BUZZER_ACTIVE_LOW = true;

// ---------------------------------------------------------------------------
// Display
// ---------------------------------------------------------------------------
static const uint32_t UI_REFRESH_MS = 200;
