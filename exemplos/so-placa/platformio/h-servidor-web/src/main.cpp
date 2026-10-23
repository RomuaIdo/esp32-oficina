#include <Arduino.h>

/*
 * Oficina de ESP32 — PETECO / UTFPR
 * Só a placa — a ESP32 cria uma rede Wi-Fi e serve uma página (prévia da oficina 4)
 *
 * Nada para montar e nenhum roteador: a placa vira um ponto de acesso.
 *
 *   1. Grave e abra o monitor serial: ele mostra o nome da rede e a senha
 *   2. No celular, conecte na rede PETECO-ESP32-xxxx (senha: oficina32)
 *   3. Abra http://192.168.4.1 no navegador
 *   4. Os botões da página ligam e desligam o LED azul da placa; a página
 *      também mostra há quanto tempo a placa está ligada e a leitura do
 *      toque no D4 (encoste o dedo e recarregue)
 *
 * O final do nome vem do MAC da placa, para várias bancadas não colidirem.
 */

#include <WiFi.h>
#include <WebServer.h>

const int PINO_LED = 2;
const int PINO_TOQUE = T0;               // T0 = GPIO 4
const char *SENHA = "oficina32";         // WPA2 exige no mínimo 8 caracteres

WebServer servidor(80);
bool ledAceso = false;
String nomeRede;

// Protótipos: a Arduino IDE os gera sozinha, o PlatformIO não.
void paginaInicial();
void ligaLed();
void desligaLed();

void setup() {
  Serial.begin(115200);
  delay(500);
  pinMode(PINO_LED, OUTPUT);

  // Últimos 2 bytes do MAC viram o sufixo do nome: PETECO-ESP32-A1B2
  uint8_t mac[6];
  WiFi.macAddress(mac);
  char sufixo[5];
  snprintf(sufixo, sizeof(sufixo), "%02X%02X", mac[4], mac[5]);
  nomeRede = String("PETECO-ESP32-") + sufixo;

  WiFi.mode(WIFI_AP);
  WiFi.softAP(nomeRede.c_str(), SENHA);

  servidor.on("/", paginaInicial);
  servidor.on("/liga", ligaLed);
  servidor.on("/desliga", desligaLed);
  servidor.begin();

  Serial.println();
  Serial.printf("rede:  %s\n", nomeRede.c_str());
  Serial.printf("senha: %s\n", SENHA);
  Serial.printf("abra:  http://%s\n", WiFi.softAPIP().toString().c_str());
}

void loop() {
  servidor.handleClient();               // atende quem pediu uma página
}

void paginaInicial() {
  String html =
    "<!doctype html><html lang='pt-BR'><head><meta charset='utf-8'>"
    "<meta name='viewport' content='width=device-width,initial-scale=1'>"
    "<title>ESP32</title><style>"
    "body{font-family:sans-serif;background:#14161a;color:#fff;text-align:center;padding:24px}"
    "h1{color:#f5c400}a{display:inline-block;margin:8px;padding:16px 28px;border-radius:8px;"
    "background:#f5c400;color:#14161a;font-weight:bold;text-decoration:none;font-size:1.2em}"
    "a.off{background:#5b6270;color:#fff}p{color:#ccc}"
    "</style></head><body>"
    "<h1>" + nomeRede + "</h1>"
    "<p>LED: <b>" + String(ledAceso ? "aceso" : "apagado") + "</b></p>"
    "<a href='/liga'>Ligar</a><a class='off' href='/desliga'>Desligar</a>"
    "<p>ligada há " + String(millis() / 1000) + " s</p>"
    "<p>toque no D4: " + String(touchRead(PINO_TOQUE)) + " (cai quando você encosta)</p>"
    "<p><a class='off' href='/'>Atualizar</a></p>"
    "</body></html>";
  servidor.send(200, "text/html; charset=utf-8", html);
}

// Liga/desliga e volta para a página inicial (redirecionamento 303).
void ligaLed() {
  ledAceso = true;
  digitalWrite(PINO_LED, HIGH);
  Serial.println("pagina pediu: LED aceso");
  servidor.sendHeader("Location", "/");
  servidor.send(303);
}

void desligaLed() {
  ledAceso = false;
  digitalWrite(PINO_LED, LOW);
  Serial.println("pagina pediu: LED apagado");
  servidor.sendHeader("Location", "/");
  servidor.send(303);
}
