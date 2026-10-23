#include <Arduino.h>

/*
 * Oficina de ESP32 — PETECO / UTFPR
 * Só a placa — pisca o LED da placa sem delay()            (tema da oficina 1)
 *
 * Nada para montar: usa o LED azul da DevKit, ligado no GPIO 2.
 *
 * Dois ritmos independentes no mesmo loop(): o LED troca a cada 500 ms e o
 * monitor serial recebe um relatório a cada 2 s. O contador de voltas mostra
 * que o loop() nunca fica parado esperando — com delay(), ele cairia para 2.
 */

const int PINO_LED = 2;                  // LED azul da DevKit V1

const unsigned long PISCA_MS = 500;
const unsigned long RELATORIO_MS = 2000;

bool ledAceso = false;
unsigned long voltas = 0;

void setup() {
  Serial.begin(115200);
  pinMode(PINO_LED, OUTPUT);
  Serial.println("piscando sem delay()");
}

void loop() {
  static unsigned long ultimoPisca = 0;
  static unsigned long ultimoRelatorio = 0;
  unsigned long agora = millis();

  if (agora - ultimoPisca >= PISCA_MS) {
    ultimoPisca = agora;
    ledAceso = !ledAceso;
    digitalWrite(PINO_LED, ledAceso);
  }

  if (agora - ultimoRelatorio >= RELATORIO_MS) {
    ultimoRelatorio = agora;
    Serial.printf("%6lu ms | voltas do loop() por segundo: %lu\n",
                  agora, voltas * 1000 / RELATORIO_MS);
    voltas = 0;
  }

  voltas++;
}
