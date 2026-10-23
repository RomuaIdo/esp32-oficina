/*
 * Oficina de ESP32 — PETECO / UTFPR
 * 04. Wi-Fi e IoT — conexão, eventos e reconexão automática
 *
 * Mostra as duas formas de conectar:
 *   bloqueante  -> simples, boa para testar
 *   por eventos -> o jeito certo num projeto de verdade
 *
 * Teste que vale fazer: com o programa rodando, desligue o roteador
 * por 10 segundos e ligue de novo. A placa precisa voltar sozinha,
 * sem você apertar reset.
 *
 * ATENÇÃO aos nomes dos eventos: no core 3.x são ARDUINO_EVENT_*.
 * Tutorial com SYSTEM_EVENT_* é da versão 2.x e não compila.
 */

#include <WiFi.h>

const char *SSID  = "PETECO-Oficina";      // troque pela rede da oficina
const char *SENHA = "esp32peteco";

// Protótipos: a Arduino IDE os gera sozinha, o PlatformIO não.
void aoObterIP(WiFiEvent_t evento, WiFiEventInfo_t info);
void aoCair(WiFiEvent_t evento, WiFiEventInfo_t info);
void mostraEstado();

// Compartilhada entre o callback (outra thread!) e o loop.
volatile bool conectado = false;

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("\n=== Wi-Fi ===");

  // Os handlers precisam ser registrados ANTES do begin(), senão você
  // pode perder o evento de conexão por uma corrida.
  WiFi.onEvent(aoObterIP, ARDUINO_EVENT_WIFI_STA_GOT_IP);
  WiFi.onEvent(aoCair,    ARDUINO_EVENT_WIFI_STA_DISCONNECTED);

  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.begin(SSID, SENHA);

  Serial.printf("conectando em \"%s\"...\n", SSID);
  // Repare: NÃO esperamos aqui. O loop() já pode começar a trabalhar.
}

void loop() {
  mostraEstado();

  // Aqui entraria o trabalho do projeto. Note que ele roda mesmo
  // enquanto o Wi-Fi ainda não conectou — que é todo o ponto.
  delay(100);
}

// ============================================================ eventos
//
// ATENÇÃO: estes callbacks rodam em OUTRA TASK do FreeRTOS, não no loop().
// Vale tudo o que vimos na oficina 3:
//   - variável compartilhada com o loop() precisa de cuidado
//   - nada de trabalho pesado aqui dentro
//   - se mexer no mesmo I2C que o loop(), use mutex
// Diferente de uma ISR, aqui Serial.print é permitido.

void aoObterIP(WiFiEvent_t evento, WiFiEventInfo_t info) {
  conectado = true;
  Serial.printf("\nconectado!  IP: %s   RSSI: %d dBm\n",
                WiFi.localIP().toString().c_str(), WiFi.RSSI());
  Serial.printf("gateway: %s   mascara: %s\n",
                WiFi.gatewayIP().toString().c_str(),
                WiFi.subnetMask().toString().c_str());
}

void aoCair(WiFiEvent_t evento, WiFiEventInfo_t info) {
  conectado = false;
  // O código do motivo ajuda a distinguir senha errada (202, 15)
  // de roteador fora do ar (200, 201).
  Serial.printf("\ncaiu! motivo %d - tentando de novo\n",
                info.wifi_sta_disconnected.reason);
  WiFi.reconnect();
}

// ============================================================ auxiliares
void mostraEstado() {
  static unsigned long ultimo = 0;

  if (millis() - ultimo < 2000) return;
  ultimo = millis();

  if (conectado) {
    Serial.printf("ok  |  IP %s  |  RSSI %d dBm  |  uptime %lus\n",
                  WiFi.localIP().toString().c_str(), WiFi.RSSI(),
                  millis() / 1000);
  } else {
    Serial.print(".");
  }
}

/*
 * Qualidade do sinal pelo RSSI (em dBm):
 *    acima de -50 .... excelente
 *    -50 a -60 ....... bom
 *    -60 a -70 ....... aceitável
 *    abaixo de -70 ... instável: aproxime a placa do roteador
 *
 * Se o projeto for montado dentro de uma caixa metálica, meça o RSSI
 * com a caixa FECHADA. O metal derruba o sinal de forma drástica.
 */
