/*
 * Oficina de ESP32 — PETECO / UTFPR
 * 01. Fundamentos — sketch de verificação do ambiente
 *
 * Grave este sketch antes de qualquer outra coisa. Se as linhas aparecerem
 * no monitor serial a 115200 baud, sua bancada está pronta.
 */

void setup() {
  Serial.begin(115200);
  delay(1000);  // dá tempo de abrir o monitor sem perder as primeiras linhas

  Serial.println("ESP32 vivo!");
  Serial.printf("Chip: %s\n", ESP.getChipModel());
  Serial.printf("Nucleos: %d | Frequencia: %d MHz\n", ESP.getChipCores(), getCpuFrequencyMhz());
  Serial.printf("RAM livre: %d bytes\n", ESP.getFreeHeap());
}

void loop() {
}
