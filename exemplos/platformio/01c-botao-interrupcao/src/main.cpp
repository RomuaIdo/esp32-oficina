#include <Arduino.h>

/*
 * Oficina de ESP32 — PETECO / UTFPR
 * 01. Fundamentos — Circuito 2: botão por interrupção
 *
 * Montagem
 *   Botão : GPIO 4 -> botão -> GND                  (usa pull-up interno)
 *   LED   : GPIO 13 -> resistor 220R -> LED -> GND
 *
 * Cada aperto do botão dispara uma interrupção que liga ou desliga o LED.
 */

const int PINO_BOTAO = 4;
const int PINO_LED = 13;

// compartilhada entre a ISR e o loop -> precisa ser volatile
volatile bool apertou = false;
bool ledAceso = false;

// IRAM_ATTR: coloca a função na RAM interna. A ISR só levanta a flag;
// nada de Serial.print ou delay aqui dentro.
void IRAM_ATTR aoApertar() {
  apertou = true;
}

void setup() {
  Serial.begin(115200);
  pinMode(PINO_LED, OUTPUT);
  pinMode(PINO_BOTAO, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(PINO_BOTAO), aoApertar, FALLING);
}

void loop() {
  if (apertou) {                  // o trabalho de verdade acontece aqui
    ledAceso = !ledAceso;
    digitalWrite(PINO_LED, ledAceso);
    Serial.println(ledAceso ? "LED aceso" : "LED apagado");

    delay(200);                   // debounce: espera o contato parar de repicar
    apertou = false;              // descarta as bordas do repique
  }
}
