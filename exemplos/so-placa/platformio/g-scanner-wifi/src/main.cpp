#include <Arduino.h>

/*
 * Oficina de ESP32 — PETECO / UTFPR
 * Só a placa — scanner de redes Wi-Fi                       (prévia da oficina 4)
 *
 * Nada para montar: lista as redes Wi-Fi ao redor a cada 10 s, com a força
 * do sinal (RSSI), o canal e o tipo de segurança. O LED fica aceso durante
 * a varredura.
 *
 * RSSI é negativo: -40 dBm é sinal ótimo, -70 é razoável, -90 mal conecta.
 */

#include <WiFi.h>

const int PINO_LED = 2;

// Protótipo: a Arduino IDE o gera sozinha, o PlatformIO não.
const char *seguranca(wifi_auth_mode_t modo);

void setup() {
  Serial.begin(115200);
  pinMode(PINO_LED, OUTPUT);
  WiFi.mode(WIFI_STA);                   // estação: só escuta, não cria rede
  WiFi.disconnect();
  delay(100);
}

void loop() {
  digitalWrite(PINO_LED, HIGH);
  Serial.println("\nprocurando redes...");
  int n = WiFi.scanNetworks();           // bloqueia por uns 2 s
  digitalWrite(PINO_LED, LOW);

  if (n <= 0) {
    Serial.println("nenhuma rede encontrada");
  } else {
    Serial.printf("%d redes:\n", n);
    Serial.println("  RSSI  canal  seguranca  nome");
    for (int i = 0; i < n; i++) {
      Serial.printf("  %4ld  %5ld  %-9s  %s\n",
                    (long)WiFi.RSSI(i), (long)WiFi.channel(i),
                    seguranca(WiFi.encryptionType(i)),
                    WiFi.SSID(i).length() ? WiFi.SSID(i).c_str() : "(oculta)");
    }
  }

  WiFi.scanDelete();                     // libera a memória da varredura
  delay(10000);
}

const char *seguranca(wifi_auth_mode_t modo) {
  switch (modo) {
    case WIFI_AUTH_OPEN:            return "aberta";
    case WIFI_AUTH_WEP:             return "WEP";
    case WIFI_AUTH_WPA_PSK:         return "WPA";
    case WIFI_AUTH_WPA2_PSK:        return "WPA2";
    case WIFI_AUTH_WPA_WPA2_PSK:    return "WPA/WPA2";
    case WIFI_AUTH_WPA3_PSK:        return "WPA3";
    case WIFI_AUTH_WPA2_WPA3_PSK:   return "WPA2/3";
    default:                        return "outra";
  }
}
