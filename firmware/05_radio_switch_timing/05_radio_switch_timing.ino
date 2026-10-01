// Measures real radio-switch overhead using the actual detector.cpp phase
// functions (not a reimplementation) - this is exactly what production
// code pays every time it switches bands. Also checks whether
// esp_wifi_set_channel accepts DFS channels (52-140) in promiscuous/STA
// mode on this core, which decides whether a "rare DFS dip" tier is even
// possible.
//
// Results go to the display, not serial - BLE radio activity appears to
// disrupt the native USB-Serial/JTAG connection (captures kept getting cut
// off right around BLE init), so the screen is the reliable channel here.

#include "detector.h"
#include "esp_wifi.h"
#include "config.h"
#include <stdarg.h>

#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>

static Adafruit_ILI9341 tft(PIN_TFT_CS, PIN_TFT_DC, PIN_TFT_RST);

static const int ITERATIONS = 8;

static uint32_t timeIt(void (*fn)()) {
  uint32_t start = millis();
  fn();
  return millis() - start;
}

static void doStopWifi()  { detectorStopWifiPhase(); }
static void doStartBle()  { detectorStartBlePhase(); }
static void doStopBle()   { detectorStopBlePhase(); }
static void doStartWifi() { detectorStartWifi5GPhase(); }

static void printLine(int16_t y, const char* fmt, ...) {
  char buf[64];
  va_list args;
  va_start(args, fmt);
  vsnprintf(buf, sizeof(buf), fmt, args);
  va_end(args);
  tft.setCursor(8, y);
  tft.print(buf);
  Serial.println(buf); // best-effort, may be lost to BLE/USB interaction
}

static void runTest() {
  uint32_t wifiToBleSum = 0, bleToWifiSum = 0;
  uint32_t wifiToBleMin = 0xFFFFFFFF, wifiToBleMax = 0;
  uint32_t bleToWifiMin = 0xFFFFFFFF, bleToWifiMax = 0;

  tft.fillScreen(ILI9341_BLACK);
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(2);
  printLine(8, "Radio switch timing");
  printLine(32, "Running %d iterations...", ITERATIONS);

  doStartWifi();
  delay(500);

  for (int i = 0; i < ITERATIONS; i++) {
    uint32_t tStopWifi = timeIt(doStopWifi);
    uint32_t tStartBle = timeIt(doStartBle);
    uint32_t wifiToBle = tStopWifi + tStartBle;
    wifiToBleSum += wifiToBle;
    if (wifiToBle < wifiToBleMin) wifiToBleMin = wifiToBle;
    if (wifiToBle > wifiToBleMax) wifiToBleMax = wifiToBle;

    delay(500); // simulate a short BLE dwell

    uint32_t tStopBle  = timeIt(doStopBle);
    uint32_t tStartWifi = timeIt(doStartWifi);
    uint32_t bleToWifi = tStopBle + tStartWifi;
    bleToWifiSum += bleToWifi;
    if (bleToWifi < bleToWifiMin) bleToWifiMin = bleToWifi;
    if (bleToWifi > bleToWifiMax) bleToWifiMax = bleToWifi;

    printLine(56 + i * 20, "i%d W>B=%lu B>W=%lu", i, wifiToBle, bleToWifi);

    delay(500); // simulate a short WiFi dwell before next switch
  }

  // --- DFS channel tunability check (WiFi already up, 5GHz) ---
  const uint8_t dfsChannels[] = {52, 100, 140};
  bool dfsOk[3];
  for (int j = 0; j < 3; j++) {
    esp_err_t err = esp_wifi_set_channel(dfsChannels[j], WIFI_SECOND_CHAN_NONE);
    uint8_t primary = 0;
    wifi_second_chan_t second;
    esp_wifi_get_channel(&primary, &second);
    dfsOk[j] = (err == ESP_OK) && (primary == dfsChannels[j]);
  }
  esp_wifi_set_channel(149, WIFI_SECOND_CHAN_NONE); // leave on a known-good channel

  tft.fillScreen(ILI9341_BLACK);
  printLine(8,  "=== RESULTS ===");
  printLine(32, "WiFi->BLE avg=%lums", wifiToBleSum / ITERATIONS);
  printLine(52, "  min=%lu max=%lu", wifiToBleMin, wifiToBleMax);
  printLine(76, "BLE->WiFi avg=%lums", bleToWifiSum / ITERATIONS);
  printLine(96, "  min=%lu max=%lu", bleToWifiMin, bleToWifiMax);
  printLine(120,"Round-trip avg=%lums", (wifiToBleSum + bleToWifiSum) / ITERATIONS);
  printLine(152,"DFS ch52=%s ch100=%s ch140=%s",
            dfsOk[0] ? "OK" : "FAIL", dfsOk[1] ? "OK" : "FAIL", dfsOk[2] ? "OK" : "FAIL");
  printLine(180,"Repeating in 20s...");
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  detectorInit();

  SPI.begin(PIN_TFT_SCK, PIN_TFT_MISO, PIN_TFT_MOSI, PIN_TFT_CS);
  tft.begin();
  tft.setRotation(1);
  tft.fillScreen(ILI9341_BLACK);
}

void loop() {
  runTest();
  delay(20000);
}
