// Bring-up step 1: blink LED_BUILTIN (GPIO27 on the XIAO ESP32-C5).
// Proves the Arduino IDE / arduino-cli toolchain and upload path with zero
// wiring. See CLAUDE.md "Bring-up order".
//
// If the LED comes on solid and blinks *off* instead of on, that's an
// active-low LED wiring - still counts as a pass, the toolchain works.

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
}

void loop() {
  digitalWrite(LED_BUILTIN, HIGH);
  delay(500);
  digitalWrite(LED_BUILTIN, LOW);
  delay(500);
}
