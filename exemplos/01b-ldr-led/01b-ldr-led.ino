/*
 * Oficina de ESP32 — PETECO / UTFPR
 * 01. Fundamentos — Circuito 1: leitura analógica com LDR
 *
 * Montagem
 *   LDR : 3V3 -> LDR -> GPIO 34 -> resistor 10k -> GND   (divisor de tensão)
 *   LED : GPIO 13 -> resistor 220R -> LED -> GND
 *
 * Lê o LDR num pino do ADC1 e acende o LED quando escurece.
 * Abra o Serial Plotter para ver a curva ao cobrir o sensor com a mão.
 */

const int PINO_LDR = 34;         // ADC1, somente entrada
const int PINO_LED = 13;
const int LIMIAR_ESCURO = 1200;  // MEÇA o seu na bancada, não copie

void setup() {
  Serial.begin(115200);
  pinMode(PINO_LED, OUTPUT);
}

void loop() {
  int bruto = analogRead(PINO_LDR);          // 0 a 4095
  int mv = analogReadMilliVolts(PINO_LDR);   // já calibrado de fábrica

  Serial.printf("bruto:%d mV:%d\n", bruto, mv);

  if (bruto < LIMIAR_ESCURO) {
    digitalWrite(PINO_LED, HIGH);   // escuro
  } else {
    digitalWrite(PINO_LED, LOW);    // claro
  }

  delay(200);
}
