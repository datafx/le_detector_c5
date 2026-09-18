// Bring-up step 2: ILI9341 display "hello world" over SPI.
// Adafruit_ILI9341 + Adafruit_GFX per CLAUDE.md decision #5 (TFT_eSPI has
// no ESP32-C5 support yet). Pins per decision #6.

#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>

#define TFT_CS   25
#define TFT_DC    7
#define TFT_RST  23
#define TFT_SCK   8
#define TFT_MISO  9
#define TFT_MOSI 10

Adafruit_ILI9341 tft(TFT_CS, TFT_DC, TFT_RST);

void setup() {
  SPI.begin(TFT_SCK, TFT_MISO, TFT_MOSI, TFT_CS);
  tft.begin();
  tft.setRotation(1);
  tft.fillScreen(ILI9341_BLACK);

  tft.setTextColor(ILI9341_GREEN);
  tft.setTextSize(3);
  tft.setCursor(10, 10);
  tft.println("LE Detector C5");

  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(2);
  tft.setCursor(10, 60);
  tft.println("Display OK");
}

void loop() {
}
