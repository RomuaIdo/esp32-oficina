#include <Arduino.h>

/*
 * Oficina de ESP32 — PETECO / UTFPR
 * 02. PWM e protocolos — LEDC: brilho, fade e som
 *
 * ATENÇÃO: este código usa a API do core 3.x.
 *   3.x (aqui):  ledcAttach(pino, freq, resolucao)  /  ledcWrite(pino, duty)
 *   2.x (velho): ledcSetup(canal, ...) + ledcAttachPin(pino, canal)
 * Se der "'ledcSetup' was not declared", o tutorial que você leu é do 2.x.
 *
 * Montagem
 *   LED    : GPIO 13 -> resistor 220R -> LED -> GND
 *   Buzzer : GPIO 25 -> buzzer PASSIVO -> GND
 *   Pot.   : 3V3 -> potenciômetro -> GND, cursor no GPIO 34 (ADC1)
 */

const int PINO_LED    = 13;
const int PINO_BUZZER = 25;
const int PINO_POT    = 34;        // ADC1: funciona mesmo com Wi-Fi ligado

// LED: 5 kHz é bem acima do que o olho percebe; 8 bits dá duty de 0 a 255.
const int LED_FREQ = 5000;
const int LED_RES  = 8;
const int LED_MAX  = (1 << LED_RES) - 1;   // 255

// protótipos: no PlatformIO, funções usadas antes de definidas precisam disto
void demonstraFade();
void beep(uint32_t freq, uint32_t ms);

void setup() {
  Serial.begin(115200);

  ledcAttach(PINO_LED, LED_FREQ, LED_RES);
  ledcAttach(PINO_BUZZER, 2000, 8);

  Serial.println("A: fade automatico | B: brilho pelo potenciometro");
  demonstraFade();
  beep(1000, 120);
}

void loop() {
  // O potenciômetro (0..4095) vira duty (0..255).
  int leitura = analogRead(PINO_POT);
  int duty    = map(leitura, 0, 4095, 0, LED_MAX);
  ledcWrite(PINO_LED, duty);

  static unsigned long ultimoLog = 0;
  if (millis() - ultimoLog >= 300) {
    ultimoLog = millis();
    Serial.printf("pot=%4d  duty=%3d/%d\n", leitura, duty, LED_MAX);
  }
}

// Sobe e desce o brilho uma vez, para mostrar o efeito do duty cycle.
void demonstraFade() {
  for (int d = 0; d <= LED_MAX; d++) { ledcWrite(PINO_LED, d); delay(4); }
  for (int d = LED_MAX; d >= 0; d--) { ledcWrite(PINO_LED, d); delay(4); }
}

// Num buzzer PASSIVO, a frequência do PWM é a própria nota.
void beep(uint32_t freq, uint32_t ms) {
  ledcWriteTone(PINO_BUZZER, freq);
  delay(ms);
  ledcWriteTone(PINO_BUZZER, 0);
}
