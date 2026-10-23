/*
 * Oficina de ESP32 — PETECO / UTFPR
 * 02. PWM e protocolos — solução da prática (partes A a D)
 *
 * Lê um sensor I2C, mostra no display OLED e usa a leitura para
 * controlar um LED por PWM. Sem nenhum delay() no loop().
 *
 * Bibliotecas (Library Manager)
 *   Adafruit SSD1306   +   Adafruit GFX   +   Adafruit BMP280
 *
 * Montagem — os dois dispositivos no MESMO barramento I2C
 *   OLED e BMP280:  VCC -> 3V3 | GND -> GND | SDA -> GPIO 21 | SCL -> GPIO 22
 *   LED:            GPIO 13 -> resistor 220R -> LED -> GND
 *
 * Antes de rodar isto, use o 02a-scanner-i2c e confirme os DOIS endereços.
 * Se o seu módulo responder em outro endereço, troque as constantes abaixo.
 */

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_BMP280.h>

// ----------------------------------------------------------- pinos e endereços
const int PINO_LED = 13;
const int PINO_SDA = 21;
const int PINO_SCL = 22;

const uint8_t END_OLED   = 0x3C;   // confirme no scanner
const uint8_t END_SENSOR = 0x76;   // alguns módulos usam 0x77

// ----------------------------------------------------------- ajustes
const int LED_FREQ = 5000;
const int LED_RES  = 8;
const int LED_MAX  = (1 << LED_RES) - 1;

// Faixa de temperatura que mapeia o brilho de 0 a 100%.
// MEÇA a sua na bancada: segurar o sensor na mão já muda vários graus.
const float TEMP_MIN = 22.0;
const float TEMP_MAX = 32.0;

const unsigned long INTERVALO_LEITURA_MS = 500;
const unsigned long INTERVALO_TELA_MS    = 200;

// ----------------------------------------------------------- objetos
Adafruit_SSD1306 display(128, 64, &Wire, -1);
Adafruit_BMP280  sensor;

float temperatura = 0.0;
float pressao     = 0.0;
int   dutyAtual   = 0;

// ----------------------------------------------------------- setup
void setup() {
  Serial.begin(115200);
  delay(300);

  Wire.begin(PINO_SDA, PINO_SCL);
  ledcAttach(PINO_LED, LED_FREQ, LED_RES);

  if (!display.begin(SSD1306_SWITCHCAPVCC, END_OLED)) {
    Serial.println("ERRO: display nao encontrado. Rode o scanner I2C.");
    while (true) delay(100);           // sem display não há o que fazer
  }

  mostraMensagem("PETECO", "iniciando...");

  if (!sensor.begin(END_SENSOR)) {
    Serial.println("ERRO: sensor nao encontrado. Rode o scanner I2C.");
    mostraMensagem("ERRO", "sensor ausente");
    while (true) delay(100);
  }

  // Modo contínuo, com filtro: boa configuração padrão do BMP280.
  sensor.setSampling(Adafruit_BMP280::MODE_NORMAL,
                     Adafruit_BMP280::SAMPLING_X2,
                     Adafruit_BMP280::SAMPLING_X16,
                     Adafruit_BMP280::FILTER_X16,
                     Adafruit_BMP280::STANDBY_MS_500);

  Serial.println("pronto");
}

// ----------------------------------------------------------- loop
void loop() {
  leSensor();
  atualizaTela();
  // nenhum delay() aqui: as duas tarefas acima têm cada uma o seu ritmo
}

// ----------------------------------------------------------- partes
void leSensor() {
  static unsigned long ultima = 0;
  if (millis() - ultima < INTERVALO_LEITURA_MS) return;
  ultima = millis();

  temperatura = sensor.readTemperature();
  pressao     = sensor.readPressure() / 100.0F;    // Pa -> hPa

  // A leitura vira brilho. constrain evita duty fora da faixa do PWM.
  float fracao = (temperatura - TEMP_MIN) / (TEMP_MAX - TEMP_MIN);
  dutyAtual = constrain((int)(fracao * LED_MAX), 0, LED_MAX);
  ledcWrite(PINO_LED, dutyAtual);

  Serial.printf("%.2f C  %.1f hPa  duty=%d\n", temperatura, pressao, dutyAtual);
}

void atualizaTela() {
  static unsigned long ultima = 0;
  if (millis() - ultima < INTERVALO_TELA_MS) return;
  ultima = millis();

  display.clearDisplay();

  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("PETECO - Oficina 2");

  display.setTextSize(2);
  display.setCursor(0, 16);
  display.printf("%.1f C", temperatura);

  display.setTextSize(1);
  display.setCursor(0, 40);
  display.printf("%.1f hPa", pressao);

  // Barra de brilho, para o duty virar algo visível no display.
  display.setCursor(0, 54);
  display.print("PWM");
  display.drawRect(30, 53, 98, 10, SSD1306_WHITE);
  display.fillRect(32, 55, map(dutyAtual, 0, LED_MAX, 0, 94), 6, SSD1306_WHITE);

  display.display();              // sem esta linha, nada aparece
}

void mostraMensagem(const char *titulo, const char *linha) {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(2);
  display.setCursor(0, 8);
  display.println(titulo);
  display.setTextSize(1);
  display.setCursor(0, 40);
  display.println(linha);
  display.display();
}
