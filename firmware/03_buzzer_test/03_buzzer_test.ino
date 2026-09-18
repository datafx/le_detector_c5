// Bring-up step 3: active piezo buzzer, one GPIO toggle.
// Buzzer signal pin per wiring plan: D0 (GPIO1).

#define BUZZER_PIN 1

void setup() {
  pinMode(BUZZER_PIN, OUTPUT);
}

void loop() {
  digitalWrite(BUZZER_PIN, HIGH);
  delay(500);
  digitalWrite(BUZZER_PIN, LOW);
  delay(500);
}
