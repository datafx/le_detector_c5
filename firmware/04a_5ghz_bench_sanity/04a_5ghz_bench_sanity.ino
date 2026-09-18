// Bench-sanity variant of the step 4 milestone test - NOT the milestone
// itself. Prints ANY 5 GHz beacon it sees (SSID, MAC, channel, RSSI),
// unfiltered, to confirm the promiscuous+5GHz capture pipeline actually
// receives packets using whatever ambient APs are nearby. Once this shows
// real beacons, swap back to 04_5ghz_promiscuous (strict 00:25:DF filter)
// for the real field test (see the private CLAUDE.md for location).

#include "WiFi.h"
#include "esp_wifi.h"
#include "esp_idf_version.h"

static const uint8_t TARGET_CHANNELS[] = {36, 40, 44, 48, 149, 153, 157, 161, 165};
static const uint16_t DWELL_MS = 300;

void IRAM_ATTR promiscuousRxCallback(void *buf, wifi_promiscuous_pkt_type_t type) {
  if (type != WIFI_PKT_MGMT) return;

  const wifi_promiscuous_pkt_t *pkt = (const wifi_promiscuous_pkt_t *)buf;
  const uint8_t *payload = pkt->payload;
  int len = pkt->rx_ctrl.sig_len;
  if (len < 38) return; // shorter than header + fixed beacon fields, skip

  uint8_t frameSubtype = payload[0] & 0xF0;
  if (frameSubtype != 0x80) return; // 0x80 = beacon

  const uint8_t *srcMac = payload + 10; // Address 2 (transmitter)
  const uint8_t *tags = payload + 24 + 12; // skip header + timestamp/interval/capability
  uint8_t tagId = tags[0];
  uint8_t tagLen = tags[1];

  char ssid[33] = {0};
  bool hidden = true;
  if (tagId == 0 && tagLen > 0 && tagLen <= 32 && (int)(tags + 2 + tagLen - payload) <= len) {
    memcpy(ssid, tags + 2, tagLen);
    hidden = false;
  }

  Serial.printf("BEACON %02X:%02X:%02X:%02X:%02X:%02X  ch %d  RSSI %d  SSID \"%s\"%s\n",
                srcMac[0], srcMac[1], srcMac[2], srcMac[3], srcMac[4], srcMac[5],
                pkt->rx_ctrl.channel, pkt->rx_ctrl.rssi,
                hidden ? "" : ssid, hidden ? "(hidden)" : "");
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("5 GHz bench sanity scan starting (all beacons, unfiltered)...");

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 4, 2)
  WiFi.setBandMode(WIFI_BAND_MODE_5G_ONLY);
#endif

  esp_wifi_set_promiscuous(true);
  esp_wifi_set_promiscuous_rx_cb(&promiscuousRxCallback);
  esp_wifi_set_channel(TARGET_CHANNELS[0], WIFI_SECOND_CHAN_NONE);
}

void loop() {
  static uint8_t idx = 0;
  idx = (idx + 1) % (sizeof(TARGET_CHANNELS) / sizeof(TARGET_CHANNELS[0]));
  esp_wifi_set_channel(TARGET_CHANNELS[idx], WIFI_SECOND_CHAN_NONE);
  delay(DWELL_MS);
}
