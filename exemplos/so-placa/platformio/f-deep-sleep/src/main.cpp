#include <Arduino.h>

/*
 * Oficina de ESP32 — PETECO / UTFPR
 * Só a placa — deep sleep, RTC RAM e Preferences            (tema da oficina 3)
 *
 * Nada para montar: acorda por tempo (10 s) ou por toque no GPIO 4 (D4).
 *
 * A cada despertar o LED pisca, o monitor mostra dois contadores e a placa
 * volta a dormir:
 *   ciclos   -> RTC_DATA_ATTR: sobrevive ao sono, ZERA no reset ou na tomada
 *   ligacoes -> Preferences:   sobrevive a tudo, mas só é gravado quando a
 *               placa liga de verdade (a flash tem escritas contadas)
 *
 * Teste: deixe dormir alguns ciclos, encoste no D4, aperte EN (reset) e
 * tire da tomada. Veja qual contador zera em cada caso.
 */

#include <Preferences.h>

const int PINO_LED = 2;
const int PINO_TOQUE = T0;               // T0 = GPIO 4
const uint64_t SEGUNDOS_DORMINDO = 10;

RTC_DATA_ATTR uint32_t ciclos = 0;       // mora na RTC RAM, que fica ligada no sono

Preferences prefs;

// Protótipos: a Arduino IDE os gera sozinha, o PlatformIO não.
const char *motivoDoDespertar(esp_sleep_wakeup_cause_t causa);
void pisca(int vezes);

void setup() {
  Serial.begin(115200);
  delay(300);                            // dá tempo ao monitor serial
  pinMode(PINO_LED, OUTPUT);

  esp_sleep_wakeup_cause_t causa = esp_sleep_get_wakeup_cause();
  ciclos++;

  // Só uma ligação de verdade (energia ou EN) grava na flash.
  prefs.begin("soplaca", false);
  uint32_t ligacoes = prefs.getUInt("ligacoes", 0);
  if (causa == ESP_SLEEP_WAKEUP_UNDEFINED) {
    ligacoes++;
    prefs.putUInt("ligacoes", ligacoes);
  }
  prefs.end();

  Serial.println();
  Serial.printf("acordei por: %s\n", motivoDoDespertar(causa));
  Serial.printf("ciclos desde o ultimo reset (RTC): %u\n", ciclos);
  Serial.printf("ligacoes desde sempre (flash):    %u\n", ligacoes);

  pisca(causa == ESP_SLEEP_WAKEUP_TOUCHPAD ? 5 : 2);

  // Limiar de toque medido agora: o valor em repouso varia entre placas.
  uint16_t repouso = touchRead(PINO_TOQUE);
  touchSleepWakeUpEnable(PINO_TOQUE, repouso * 2 / 3);
  esp_sleep_enable_timer_wakeup(SEGUNDOS_DORMINDO * 1000000ULL);

  Serial.printf("dormindo %llu s (ou ate tocar no D4)...\n", SEGUNDOS_DORMINDO);
  Serial.flush();                        // esvazia a serial ANTES de dormir
  esp_deep_sleep_start();
}

void loop() {
  // Nunca chega aqui: o deep sleep sempre recomeça pelo setup().
}

const char *motivoDoDespertar(esp_sleep_wakeup_cause_t causa) {
  switch (causa) {
    case ESP_SLEEP_WAKEUP_TIMER:    return "timer";
    case ESP_SLEEP_WAKEUP_TOUCHPAD: return "toque no D4";
    default:                        return "ligacao ou reset (EN)";
  }
}

void pisca(int vezes) {
  for (int i = 0; i < vezes; i++) {
    digitalWrite(PINO_LED, HIGH); delay(100);
    digitalWrite(PINO_LED, LOW);  delay(100);
  }
}
