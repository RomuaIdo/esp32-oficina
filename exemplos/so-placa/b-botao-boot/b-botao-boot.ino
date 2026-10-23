/*
 * Oficina de ESP32 — PETECO / UTFPR
 * Só a placa — botão BOOT por interrupção                   (tema da oficina 1)
 *
 * Nada para montar: o botão BOOT da DevKit fica no GPIO 0, que já tem um
 * resistor de pull-up na placa. Solto ele lê HIGH; apertado, LOW.
 *
 * Cada aperto dispara uma interrupção que só levanta uma flag. O loop()
 * faz o trabalho de verdade: alterna o LED e conta os apertos. O debounce
 * usa millis(), sem delay(), então o loop() continua livre.
 *
 * Use o BOOT, não o EN: o EN é o reset da placa.
 */

const int PINO_BOTAO = 0;                // botão BOOT
const int PINO_LED = 2;                  // LED azul da DevKit V1
const unsigned long DEBOUNCE_MS = 200;

// compartilhada entre a ISR e o loop -> precisa ser volatile
volatile bool apertou = false;

bool ledAceso = false;
unsigned int apertos = 0;

// IRAM_ATTR: a ISR fica na RAM interna. Nada de Serial.print aqui dentro.
void IRAM_ATTR aoApertar() {
  apertou = true;
}

void setup() {
  Serial.begin(115200);
  pinMode(PINO_LED, OUTPUT);
  pinMode(PINO_BOTAO, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(PINO_BOTAO), aoApertar, FALLING);
  Serial.println("aperte o botao BOOT");
}

void loop() {
  static unsigned long ultimoAperto = 0;

  if (apertou) {
    apertou = false;

    // O contato repica: bordas a menos de 200 ms do último aperto são ruído.
    if (millis() - ultimoAperto >= DEBOUNCE_MS) {
      ultimoAperto = millis();
      apertos++;
      ledAceso = !ledAceso;
      digitalWrite(PINO_LED, ledAceso);
      Serial.printf("aperto #%u -> LED %s\n", apertos, ledAceso ? "aceso" : "apagado");
    }
  }
}
