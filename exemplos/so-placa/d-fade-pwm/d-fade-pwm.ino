/*
 * Oficina de ESP32 — PETECO / UTFPR
 * Só a placa — LED "respirando" por PWM                     (tema da oficina 2)
 *
 * Nada para montar: usa o LED azul da DevKit (GPIO 2).
 *
 * O LEDC gera o PWM; o duty cycle sobe e desce suavemente. Cada aperto no
 * botão BOOT troca a velocidade da respiração. API do core 3.x:
 *   ledcAttach(pino, freq, resolucao)  e  ledcWrite(pino, duty)
 */

const int PINO_LED = 2;
const int PINO_BOTAO = 0;                // botão BOOT

const int LED_FREQ = 5000;               // 5 kHz: o olho não vê a piscada
const int LED_RES = 8;                   // duty de 0 a 255
const int LED_MAX = (1 << LED_RES) - 1;

const int PASSOS_MS[] = { 8, 3, 1 };     // devagar, médio, rápido
const int N_VELOCIDADES = sizeof(PASSOS_MS) / sizeof(PASSOS_MS[0]);
int velocidade = 0;

void setup() {
  Serial.begin(115200);
  pinMode(PINO_BOTAO, INPUT_PULLUP);
  ledcAttach(PINO_LED, LED_FREQ, LED_RES);
  Serial.println("aperte BOOT para trocar a velocidade");
}

void loop() {
  static unsigned long ultimoPasso = 0;
  static int duty = 0;
  static int sentido = 1;

  // Um passo de brilho a cada PASSOS_MS[velocidade] milissegundos.
  if (millis() - ultimoPasso >= (unsigned long)PASSOS_MS[velocidade]) {
    ultimoPasso = millis();
    duty += sentido;
    if (duty >= LED_MAX || duty <= 0) sentido = -sentido;
    ledcWrite(PINO_LED, duty);
  }

  // Botão por leitura simples: detecta a borda de descida (solto -> apertado).
  static bool estavaApertado = false;
  static unsigned long ultimoAperto = 0;
  bool apertado = digitalRead(PINO_BOTAO) == LOW;
  if (apertado && !estavaApertado && millis() - ultimoAperto > 200) {
    ultimoAperto = millis();
    velocidade = (velocidade + 1) % N_VELOCIDADES;
    Serial.printf("velocidade %d de %d\n", velocidade + 1, N_VELOCIDADES);
  }
  estavaApertado = apertado;
}
