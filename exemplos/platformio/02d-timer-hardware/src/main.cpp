#include <Arduino.h>

/*
 * Oficina de ESP32 — PETECO / UTFPR
 * 02. PWM e protocolos — timer de hardware (desafio da prática)
 *
 * Amostra um sensor em intervalo EXATO, sem depender de o loop() passar ali.
 *
 * ATENÇÃO: API do core 3.x.
 *   3.x (aqui):  timerBegin(frequencia) + timerAlarm(timer, valor, auto, cont)
 *   2.x (velho): timerBegin(num, divisor, contaPraCima) + timerAlarmWrite/Enable
 *
 * Montagem
 *   LDR ou potenciômetro no GPIO 34 (ADC1)
 */

const int PINO_SENSOR = 34;

hw_timer_t *timer = NULL;

// Compartilhadas entre a ISR e o loop: precisam ser volatile.
volatile bool     temAmostra = false;
volatile uint32_t amostra    = 0;
volatile uint32_t totalAmostras = 0;

// A ISR do timer segue as MESMAS regras da ISR de GPIO:
// IRAM_ATTR, nada de Serial.print, nada de delay, nada de alocação.
void IRAM_ATTR aoEstourarTimer() {
  amostra = analogRead(PINO_SENSOR);
  totalAmostras = totalAmostras + 1;   // "++" em volatile gera aviso no C++20
  temAmostra = true;
}

void setup() {
  Serial.begin(115200);
  delay(300);

  // Base de 1 MHz: cada tique do contador vale 1 microssegundo.
  timer = timerBegin(1000000);
  timerAttachInterrupt(timer, &aoEstourarTimer);

  // Dispara a cada 100.000 tiques = 100.000 us = 100 ms = 10 Hz.
  //   true = recarrega sozinho    0 = repete para sempre
  timerAlarm(timer, 100000, true, 0);

  Serial.println("amostrando a 10 Hz por timer de hardware");
}

void loop() {
  if (temAmostra) {
    temAmostra = false;                 // consome a flag
    uint32_t valor = amostra;           // copia antes de usar

    Serial.printf("#%lu  valor=%4lu\n", totalAmostras, valor);
  }

  // Este atraso proposital mostra a diferença: mesmo com o loop lento,
  // a amostragem continua saindo a 10 Hz certinhos, porque quem conta
  // é o periférico, não o loop(). Com millis(), ela atrasaria junto.
  delay(250);
}
