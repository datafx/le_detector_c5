/*
    LE Gear Detector - scanning core

    BLE scan setup / GAP callback structure and the WiFi promiscuous handler
    trace back to nyanBOX (axon_detector.cpp, device_scout.cpp)
    https://github.com/jbohack/nyanBOX
    Copyright (c) 2025 jbohack - MIT License
    SPDX-License-Identifier: MIT

    Changes from the le_detector (2.4GHz-only) version this was ported from:
      - Single-band WiFi phase split into separate 5GHz and 2.4GHz phases,
        each switching band mode on entry via esp_wifi_set_band_mode() - the
        C5 has one radio behind a dual-band diplexer, so band is a runtime
        switch, not a compile-time assumption.
      - Channel hop now picks its channel list/interval based on which band
        is active (WIFI_5G_CHANNELS vs 1-WIFI_2G_MAX_CHANNEL).
      - Packet parsing, OUI/SSID matching, and TrackedDevice tracking are
        unchanged - all already band-agnostic.
      - BLE scanning ported from raw Bluedroid GAP calls to the Arduino
        BLEDevice/BLEScan wrapper - Bluedroid doesn't exist on this (or any
        RISC-V ESP32) target, only NimBLE does, and the wrapper covers both.
        Same passive/public-address-only matching behavior as before.
*/

#include "detector.h"
#include "config.h"
#include "ssid_table.h"

#include "esp_wifi.h"
#include "esp_netif.h"
#include "esp_idf_version.h"

#include <BLEDevice.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>

static TrackedDevice s_devices[MAX_TRACKED];
static uint8_t       s_channel = 1;
static uint8_t       s_channelIdx = 0;   // index into WIFI_5G_CHANNELS when 5GHz is active
static uint32_t      s_lastHop = 0;
static uint16_t      s_seenTotal = 0;
static uint32_t      s_lastHitMs = 0;

enum WifiBand : uint8_t { BAND_5G, BAND_2G };
static WifiBand s_band = BAND_5G;

static bool s_wifiUp = false;
static bool s_bleUp  = false;

// ---------------------------------------------------------------------------
// Device table
// ---------------------------------------------------------------------------

// vendor/category come straight from either an OuiEntry or an SsidEntry -
// taking them by value instead of an OuiEntry* lets both matchers share this
// one recordMatch path. `ssid` is optional (nullptr/omitted for OUI and BLE
// matches, which have none) and is copied, not stored by pointer, since it
// points at a stack buffer that doesn't outlive the calling frame.
static void recordMatch(const uint8_t mac[6], int8_t rssi,
                        const char* vendor, GearCategory category,
                        Source src, const char* ssid = nullptr) {
    uint32_t now = millis();
    s_lastHitMs = now;

    // Existing entry?
    for (uint8_t i = 0; i < MAX_TRACKED; i++) {
        if (!s_devices[i].used) continue;
        if (memcmp(s_devices[i].mac, mac, 6) != 0) continue;

        TrackedDevice& d = s_devices[i];
        // EMA so a single weak/strong frame doesn't swing the flash rate
        d.rssi = (int16_t)((rssi * RSSI_EMA_NUM +
                            d.rssi * (RSSI_EMA_DEN - RSSI_EMA_NUM)) / RSSI_EMA_DEN);
        d.lastSeen = now;
        return;
    }

    // New entry - take a free slot, else evict the least recently seen.
    uint8_t slot = 0xFF;
    for (uint8_t i = 0; i < MAX_TRACKED; i++) {
        if (!s_devices[i].used) { slot = i; break; }
    }
    if (slot == 0xFF) {
        uint32_t oldest = 0xFFFFFFFF;
        for (uint8_t i = 0; i < MAX_TRACKED; i++) {
            if (s_devices[i].lastSeen < oldest) { oldest = s_devices[i].lastSeen; slot = i; }
        }
    }

    TrackedDevice& d = s_devices[slot];
    memcpy(d.mac, mac, 6);
    d.rssi      = rssi;
    d.firstSeen = now;
    d.lastSeen  = now;
    d.vendor    = vendor;
    d.category  = category;
    d.source    = src;
    if (ssid) {
        strncpy(d.ssid, ssid, sizeof(d.ssid) - 1);
        d.ssid[sizeof(d.ssid) - 1] = '\0';
    } else {
        d.ssid[0] = '\0';
    }
    d.used      = true;
}

void detectorExpire() {
    uint32_t now = millis();
    for (uint8_t i = 0; i < MAX_TRACKED; i++) {
        if (!s_devices[i].used) continue;
        if (now - s_devices[i].lastSeen > DEVICE_TTL_MS) {
            s_devices[i].used = false;
        }
    }
}

DetectorStatus detectorStatus() {
    DetectorStatus st = {};
    st.bestRssi  = -127;
    st.best      = nullptr;
    st.lastHitMs = s_lastHitMs;

    for (uint8_t i = 0; i < MAX_TRACKED; i++) {
        if (!s_devices[i].used) continue;
        st.activeCount++;
        if (s_devices[i].rssi > st.bestRssi) {
            st.bestRssi = s_devices[i].rssi;
            st.best     = &s_devices[i];
        }
    }
    return st;
}

uint8_t  detectorChannel()   { return s_channel; }
uint16_t detectorSeenTotal() { return s_seenTotal; }

// ---------------------------------------------------------------------------
// WiFi promiscuous sniffing (band-agnostic - operates on whatever channel
// the radio is currently tuned to)
// ---------------------------------------------------------------------------

// Pulls the SSID information element out of a beacon / probe request / probe
// response frame. Per 802.11, SSID (element ID 0x00) is always the first IE
// following the frame's fixed fields, so this is a direct offset check, not
// a general IE walk - probe requests have no fixed fields between the
// 24-byte MAC header and the IEs, beacons/probe responses have 12
// (8-byte timestamp + 2-byte interval + 2-byte capability info).
//
// Every length is checked against `len` before being read - the length byte
// inside the frame is attacker/interference-controlled and is never trusted
// on its own. Returns false (no bytes emitted) for a missing/malformed IE, a
// zero-length SSID (wildcard probe or hidden-network beacon - nothing to
// match either way), or an SSID longer than the 32-byte spec maximum.
static bool extractSsid(const uint8_t* frame, int len, uint8_t subtype,
                        const uint8_t** ssidBytes, uint8_t* ssidLen) {
    int ieOffset = (subtype == 0x40) ? 24 : 36;
    if (len < ieOffset + 2) return false;
    if (frame[ieOffset] != 0x00) return false;        // not the SSID IE
    uint8_t l = frame[ieOffset + 1];
    if (l == 0 || l > 32) return false;
    if (len < ieOffset + 2 + (int)l) return false;    // length byte not trusted

    *ssidBytes = &frame[ieOffset + 2];
    *ssidLen   = l;
    return true;
}

static void IRAM_ATTR wifiSnifferCb(void* buff, wifi_promiscuous_pkt_type_t type) {
    if (type != WIFI_PKT_MGMT) return;

    const wifi_promiscuous_pkt_t* ppkt = (wifi_promiscuous_pkt_t*)buff;
    const uint8_t* frame = ppkt->payload;
    int len = ppkt->rx_ctrl.sig_len;
    if (len < 24) return;   // shorter than a full mgmt header - nothing usable

    uint8_t subtype = frame[0] & 0xF0;
    // 0x80 beacon, 0x40 probe request, 0x50 probe response
    if (subtype != 0x80 && subtype != 0x40 && subtype != 0x50) return;

    // addr2 = transmitter address
    const uint8_t* mac = &frame[10];
    if (mac[0] & 0x01) return;   // multicast/broadcast, not a real transmitter

    s_seenTotal++;

    // OUI match - meaningless on a randomised address, so skip it there.
    if (!(mac[0] & 0x02)) {
        const OuiEntry* oui = ouiLookup(mac);
        if (oui) recordMatch(mac, ppkt->rx_ctrl.rssi, oui->vendor, oui->category, SRC_WIFI);
    }

    // SSID match - runs regardless of address randomisation. A directed
    // probe request leaks its target SSID in cleartext as an 802.11 protocol
    // requirement, not a configuration choice, so it survives the same MAC
    // rotation that defeats OUI matching.
    const uint8_t* ssidBytes;
    uint8_t ssidLen;
    if (extractSsid(frame, len, subtype, &ssidBytes, &ssidLen)) {
        char ssid[33];
        memcpy(ssid, ssidBytes, ssidLen);
        ssid[ssidLen] = '\0';

        const SsidEntry* hit = ssidLookup(ssid);
        if (hit) {
            // Probe requests are weaker evidence (a client leaking a
            // previously-joined network, not proof the AP is here now) -
            // beacons/probe responses mean the AP itself is transmitting.
            Source src = (subtype == 0x40) ? SRC_PROBE : SRC_WIFI;
            recordMatch(mac, ppkt->rx_ctrl.rssi, hit->vendor, hit->category, src, ssid);
        }
    }
}

void detectorHopChannel() {
    uint32_t now = millis();
    uint32_t hopMs = (s_band == BAND_5G) ? WIFI_5G_HOP_MS : WIFI_2G_HOP_MS;
    if (now - s_lastHop < hopMs) return;

    if (s_band == BAND_5G) {
        s_channelIdx = (s_channelIdx + 1) % WIFI_5G_CHANNEL_COUNT;
        s_channel = WIFI_5G_CHANNELS[s_channelIdx];
    } else {
        s_channel = (s_channel >= WIFI_2G_MAX_CHANNEL) ? 1 : (s_channel + 1);
    }
    esp_wifi_set_channel(s_channel, WIFI_SECOND_CHAN_NONE);
    s_lastHop = now;
}

static void startWifiCommon() {
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);
    esp_wifi_set_storage(WIFI_STORAGE_RAM);
    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_start();

    esp_wifi_set_ps(WIFI_PS_NONE);

    wifi_promiscuous_filter_t flt = {};
    flt.filter_mask = WIFI_PROMIS_FILTER_MASK_MGMT;
    esp_wifi_set_promiscuous_filter(&flt);
    esp_wifi_set_promiscuous_rx_cb(&wifiSnifferCb);
    esp_wifi_set_promiscuous(true);

    s_lastHop = millis();
    s_wifiUp = true;
}

void detectorStartWifi5GPhase() {
    if (s_wifiUp) return;
    s_band = BAND_5G;
    startWifiCommon();
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 4, 2)
    esp_wifi_set_band_mode(WIFI_BAND_MODE_5G_ONLY);
#endif
    s_channelIdx = 0;
    s_channel = WIFI_5G_CHANNELS[0];
    esp_wifi_set_channel(s_channel, WIFI_SECOND_CHAN_NONE);
}

void detectorStartWifi2GPhase() {
    if (s_wifiUp) return;
    s_band = BAND_2G;
    startWifiCommon();
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 4, 2)
    esp_wifi_set_band_mode(WIFI_BAND_MODE_2G_ONLY);
#endif
    s_channel = 1;
    esp_wifi_set_channel(s_channel, WIFI_SECOND_CHAN_NONE);
}

void detectorStopWifiPhase() {
    if (!s_wifiUp) return;
    esp_wifi_set_promiscuous(false);
    esp_wifi_stop();
    delay(50);
    esp_wifi_deinit();
    delay(50);
    s_wifiUp = false;
}

// ---------------------------------------------------------------------------
// BLE scanning
//
// The classic Bluedroid GAP API (esp_gap_ble_api.h / esp_bt_main.h) this was
// originally written against only ships for Xtensa ESP32 - every RISC-V
// variant (C3/C5/C6/H2), this board included, uses NimBLE as its BLE host
// stack instead. The bundled Arduino BLEDevice/BLEScan/BLEAdvertisedDevice
// classes wrap both backends behind one API, so that's what this uses -
// same passive-scan, public-address-only matching behavior as before, just
// through the portable wrapper instead of a Bluedroid-specific call.
// ---------------------------------------------------------------------------

static BLEScan* s_bleScan = nullptr;

class DetectorAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks {
    void onResult(BLEAdvertisedDevice advertisedDevice) override {
        // Address type 0x00 is "Public Device Address" per the Bluetooth
        // Core Spec, same numeric value under both Bluedroid and NimBLE.
        // Random / resolvable-private addresses (anything else) have
        // randomised vendor bits, so matching them is meaningless.
        if (advertisedDevice.getAddressType() != 0) return;

        s_seenTotal++;

        uint8_t mac[6];
        memcpy(mac, advertisedDevice.getAddress().getNative(), 6);
        const OuiEntry* oui = ouiLookup(mac);
        if (oui) recordMatch(mac, (int8_t)advertisedDevice.getRSSI(), oui->vendor, oui->category, SRC_BLE);
    }
};

void detectorStartBlePhase() {
    if (s_bleUp) return;

    if (!BLEDevice::getInitialized()) {
        BLEDevice::init("");
    }

    s_bleScan = BLEDevice::getScan();
    s_bleScan->setAdvertisedDeviceCallbacks(new DetectorAdvertisedDeviceCallbacks(), true);
    s_bleScan->setActiveScan(!BLE_SCAN_PASSIVE);  // locked decision: stays passive
    s_bleScan->setInterval(0x100);
    s_bleScan->setWindow(0xA0);
    s_bleScan->start(0, nullptr, false);  // duration 0 = scan until told to stop

    s_bleUp = true;
}

void detectorStopBlePhase() {
    if (!s_bleUp) return;
    if (s_bleScan) s_bleScan->stop();
    s_bleUp = false;
}

void detectorInit() {
    memset(s_devices, 0, sizeof(s_devices));
    s_seenTotal = 0;
    s_lastHitMs = 0;
}
