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
// 5 GHz is weighted heaviest - the Axon Fleet Hub beacons continuously there
// and is the primary target found in the investigation. Channel list is the
// 9 non-DFS 5 GHz channels (36/40/44/48/149/153/157/161/165) rather than the
// full 36-165 range - DFS channels (52-144) need radar-detection handling
// this fork doesn't do yet, and every confirmed real-world target so far
// (Axon Fleet Hub, general enterprise gear) sits on non-DFS channels anyway.
// Revisit if that assumption doesn't hold up in the field.
// ---------------------------------------------------------------------------
static const uint8_t WIFI_5G_CHANNELS[]  = {36, 40, 44, 48, 149, 153, 157, 161, 165};
static const uint8_t WIFI_5G_CHANNEL_COUNT = sizeof(WIFI_5G_CHANNELS) / sizeof(WIFI_5G_CHANNELS[0]);
static const uint32_t WIFI_5G_PHASE_MS   = 4500;  // ~1.5 sweeps of 9 channels at 300ms hop
static const uint32_t WIFI_5G_HOP_MS     = 300;

// US 2.4GHz WiFi is FCC-licensed on channels 1-11 only; 12-13 are ETSI-region
// channels no US-market AP will legally beacon on, so scanning them is wasted
// dwell time.
static const uint8_t  WIFI_2G_MAX_CHANNEL = 11;
static const uint32_t WIFI_2G_PHASE_MS    = 3000;  // ~1 full sweep of ch 1-11, plus slack
static const uint32_t WIFI_2G_HOP_MS      = 250;

static const uint32_t BLE_PHASE_MS        = 3000;

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
static const int8_t   RSSI_STRONG       = -45;   // maps to the fastest flash
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
