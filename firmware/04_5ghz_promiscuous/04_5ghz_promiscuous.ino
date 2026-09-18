// Bring-up step 4: THE MILESTONE - 5 GHz promiscuous scan.
// Channels 153/157/165, print any MAC beginning 00:25:DF (Axon) to serial.
// Nothing else. This single test determines whether the fork's entire
// premise holds - do it before investing in UI or alert logic.
//
// Field-test location and known-good ground truth (specific captured MACs,
// exact coordinates) are kept in the private, local-only CLAUDE.md rather
// than here - this file is public.

#include "WiFi.h"
#include "esp_wifi.h"
#include "esp_idf_version.h"

static const uint8_t TARGET_CHANNELS[] = {153, 157, 165};
static const uint8_t TARGET_PREFIX[3] = {0x00, 0x25, 0xDF};
static const uint16_t DWELL_MS = 400;

void IRAM_ATTR promiscuousRxCallback(void *buf, wifi_promiscuous_pkt_type_t type) {
  if (type != WIFI_PKT_MGMT) return;

  const wifi_promiscuous_pkt_t *pkt = (const wifi_promiscuous_pkt_t *)buf;
  const uint8_t *payload = pkt->payload;
  const uint8_t *srcMac = payload + 10; // 802.11 Address 2 (transmitter)

  if (srcMac[0] == TARGET_PREFIX[0] &&
      srcMac[1] == TARGET_PREFIX[1] &&
      srcMac[2] == TARGET_PREFIX[2]) {
    Serial.printf("MATCH %02X:%02X:%02X:%02X:%02X:%02X  ch %d  RSSI %d\n",
                  srcMac[0], srcMac[1], srcMac[2], srcMac[3], srcMac[4], srcMac[5],
                  pkt->rx_ctrl.channel, pkt->rx_ctrl.rssi);
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("5 GHz promiscuous scan starting (ch 153/157/165)...");

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
