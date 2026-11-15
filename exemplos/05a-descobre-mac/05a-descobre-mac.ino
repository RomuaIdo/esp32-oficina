/*
 * Oficina de ESP32 — PETECO / UTFPR
 * 05. ESP-NOW, BLE e integração — descobrir o endereço MAC
 *
 * O ESP-NOW endereça pelo MAC, não por IP. Grave isto em TODA placa
 * antes da prática e anote o resultado — de preferência numa etiqueta
 * colada na própria placa.
 *
 * O MAC é gravado de fábrica e não muda nunca.
 *
 * Saída esperada:
 *   MAC (copie para o codigo): A0:B7:65:1C:2D:EF
 *   Formato C: {0xA0, 0xB7, 0x65, 0x1C, 0x2D, 0xEF}
 */

#include <WiFi.h>
#include <esp_wifi.h>

void setup() {
  Serial.begin(115200);
  delay(600);

  // Precisa estar em modo estação para o MAC da interface Wi-Fi existir.
  WiFi.mode(WIFI_STA);

  Serial.println();
  Serial.println("========================================");
  Serial.print("MAC (copie para o codigo): ");
  Serial.println(WiFi.macAddress());

  // Mesma coisa, já no formato que você vai colar no array do emissor.
  uint8_t mac[6];
  esp_wifi_get_mac(WIFI_IF_STA, mac);

  Serial.print("Formato C: {");
  for (int i = 0; i < 6; i++) {
    Serial.printf("0x%02X%s", mac[i], i < 5 ? ", " : "");
  }
  Serial.println("}");

  // O canal importa muito no ESP-NOW: se as placas estiverem em canais
  // diferentes, nada chega e não aparece nenhum erro.
  Serial.printf("Canal atual: %d\n", WiFi.channel());
  Serial.println("========================================");
}

void loop() {
  delay(10000);
}
