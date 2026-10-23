#include <Arduino.h>

/*
 * Oficina de ESP32 — PETECO / UTFPR
 * 02. PWM e protocolos — scanner I2C
 *
 * Rode ISTO antes de qualquer biblioteca de sensor I2C.
 * Ele responde a pergunta que importa: o dispositivo está respondendo?
 *
 * Montagem
 *   VCC do módulo -> 3V3    (confira a tensão do SEU módulo)
 *   GND do módulo -> GND
 *   SDA do módulo -> GPIO 21
 *   SCL do módulo -> GPIO 22
 *
 * Como ler o resultado
 *   aparece o endereço esperado .... hardware OK, problema é software
 *   nenhum dispositivo ............. fio trocado, sem alimentação,
 *                                    SDA/SCL invertidos ou pull-up ausente
 *   endereço diferente ............. variante do módulo: use o que apareceu
 *   todos os 127 endereços ......... SDA em curto com GND, ou sem pull-up
 */

#include <Wire.h>

const int PINO_SDA = 21;
const int PINO_SCL = 22;

const char *descreve(uint8_t endereco);   // protótipo: o PlatformIO exige

void setup() {
  Serial.begin(115200);
  delay(500);

  Wire.begin(PINO_SDA, PINO_SCL);

  Serial.println();
  Serial.println("=== Scanner I2C ===");
}

void loop() {
  int achados = 0;

  for (uint8_t endereco = 1; endereco < 127; endereco++) {
    Wire.beginTransmission(endereco);
    uint8_t erro = Wire.endTransmission();

    if (erro == 0) {               // 0 = o dispositivo deu ACK
      Serial.printf("  dispositivo em 0x%02X", endereco);
      Serial.println(descreve(endereco));
      achados++;
    }
  }

  if (achados == 0) {
    Serial.println("  nenhum dispositivo encontrado");
    Serial.println("  confira: alimentacao, SDA/SCL trocados, pull-ups");
  } else {
    Serial.printf("  total: %d dispositivo(s)\n", achados);
  }

  Serial.println("-------------------");
  delay(5000);                     // repete, para você mexer nos fios e ver
}

// Palpite de qual módulo é, pelos endereços mais comuns em bancada.
const char *descreve(uint8_t endereco) {
  switch (endereco) {
    case 0x3C:
    case 0x3D: return "  (provavel display OLED SSD1306)";
    case 0x68: return "  (provavel MPU6050 ou RTC DS3231)";
    case 0x76:
    case 0x77: return "  (provavel BMP280 / BME280)";
    case 0x27:
    case 0x3F: return "  (provavel display LCD com backpack)";
    case 0x48: return "  (provavel ADS1115)";
    default:   return "";
  }
}
