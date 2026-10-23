/*
 * Oficina de ESP32 — PETECO / UTFPR
 * Só a placa — sensor de toque capacitivo                   (tema da oficina 1)
 *
 * Nada para montar: encoste o dedo no pino GPIO 4 (marcado D4 na placa).
 * Um jumper espetado nele vira uma "antena" mais fácil de tocar.
 *
 * O ESP32 mede a capacitância do pino. Quando o dedo encosta, a leitura
 * de touchRead() CAI. O programa mede o valor em repouso ao ligar e acende
 * o LED quando a leitura fica bem abaixo disso.
 *
 * Abra o Serial Plotter (ou o Teleplot, no PlatformIO) para ver a curva.
 * Não toque no pino nos primeiros 2 segundos: é a calibração.
 */

const int PINO_TOQUE = T0;               // T0 = GPIO 4
const int PINO_LED = 2;                  // LED azul da DevKit V1

int repouso = 0;                         // leitura sem toque, medida no setup
int limiar = 0;

void setup() {
  Serial.begin(115200);
  pinMode(PINO_LED, OUTPUT);
  delay(500);

  // Média de várias leituras: o valor em repouso muda de placa para placa
  // e com o comprimento do fio, por isso não dá para fixar no código.
  long soma = 0;
  for (int i = 0; i < 20; i++) {
    soma += touchRead(PINO_TOQUE);
    delay(50);
  }
  repouso = soma / 20;
  limiar = repouso * 2 / 3;              // tocou = caiu mais de um terço

  Serial.printf("repouso: %d | limiar de toque: %d\n", repouso, limiar);
}

void loop() {
  int leitura = touchRead(PINO_TOQUE);
  bool tocando = leitura < limiar;

  digitalWrite(PINO_LED, tocando);

  // formato que o Serial Plotter e o Teleplot entendem
  Serial.printf(">leitura:%d\n>limiar:%d\n", leitura, limiar);
  delay(50);
}
