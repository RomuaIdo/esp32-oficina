/*
 * Oficina de ESP32 — PETECO / UTFPR
 * 04. Wi-Fi e IoT — servidor web com painel que atualiza sozinho
 *
 * Três rotas:
 *   GET /            -> a página (HTML + JavaScript)
 *   GET /api         -> a leitura atual, em JSON
 *   GET /led?estado=1 -> liga (1) ou desliga (0) o LED
 *
 * A página é servida UMA vez. Depois o JavaScript dela busca só o JSON
 * de /api a cada segundo. É assim que o painel atualiza sem recarregar
 * e sem a placa remontar HTML o tempo todo.
 *
 * Montagem
 *   Sensor : LDR ou potenciômetro no GPIO 34 (ADC1)
 *   LED    : GPIO 13 -> resistor 220R -> LED -> GND
 *
 * Como usar: abra o IP que aparecer no monitor serial, pelo celular,
 * com o celular NA MESMA REDE (não nos dados móveis).
 */

#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>

const char *SSID  = "PETECO-Oficina";
const char *SENHA = "esp32peteco";
const char *NOME_MDNS = "peteco";          // vira http://peteco.local

const int PINO_SENSOR = 34;
const int PINO_LED    = 13;

WebServer server(80);
bool ledLigado = false;

// Protótipos: a Arduino IDE os gera sozinha, o PlatformIO não.
void rotaRaiz();
void rotaApi();
void rotaLed();

// A página vive na flash (PROGMEM), não na RAM. Para algo maior que
// isto, o certo é servir do LittleFS em vez de embutir no código.
const char PAGINA[] PROGMEM = R"HTML(
<!DOCTYPE html><html lang="pt-BR">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width,initial-scale=1">
  <title>PETECO - Oficina 4</title>
  <style>
    body { font-family: system-ui, sans-serif; background:#14161a; color:#fff;
           margin:0; padding:24px; text-align:center; }
    h1 { color:#f5c400; font-size:22px; margin:0 0 24px; }
    .valor { font-size:64px; font-weight:800; margin:16px 0; }
    .rotulo { color:#8b93a1; font-size:14px; text-transform:uppercase;
              letter-spacing:.1em; }
    button { font-size:18px; padding:14px 32px; margin-top:28px; border:none;
             border-radius:8px; background:#f5c400; font-weight:700;
             cursor:pointer; }
    .off { background:#5b6270; color:#fff; }
    .rodape { color:#5b6270; font-size:12px; margin-top:32px; }
  </style>
</head>
<body>
  <h1>PETECO &middot; Oficina de ESP32</h1>

  <div class="rotulo">Leitura do sensor</div>
  <div class="valor" id="valor">--</div>

  <button id="btn" onclick="alterna()">carregando...</button>

  <div class="rodape">ligado ha <span id="uptime">0</span> s</div>

<script>
let ligado = false;

async function atualiza() {
  try {
    const d = await (await fetch('/api')).json();
    document.getElementById('valor').textContent  = d.sensor;
    document.getElementById('uptime').textContent = d.uptime;
    ligado = d.led;
    pintaBotao();
  } catch (e) {
    document.getElementById('valor').textContent = '--';
  }
}

function pintaBotao() {
  const b = document.getElementById('btn');
  b.textContent = ligado ? 'DESLIGAR LED' : 'LIGAR LED';
  b.className   = ligado ? '' : 'off';
}

async function alterna() {
  await fetch('/led?estado=' + (ligado ? 0 : 1));
  atualiza();
}

setInterval(atualiza, 1000);
atualiza();
</script>
</body></html>
)HTML";

// ============================================================ setup
void setup() {
  Serial.begin(115200);
  delay(500);

  pinMode(PINO_LED, OUTPUT);

  WiFi.mode(WIFI_STA);
  WiFi.begin(SSID, SENHA);
  Serial.print("conectando");
  while (WiFi.status() != WL_CONNECTED) { delay(300); Serial.print("."); }

  Serial.printf("\n\n  abra no navegador:  http://%s\n",
                WiFi.localIP().toString().c_str());

  if (MDNS.begin(NOME_MDNS)) {
    MDNS.addService("http", "tcp", 80);
    Serial.printf("  ou, se o seu sistema suportar:  http://%s.local\n\n",
                  NOME_MDNS);
  }

  server.on("/",    rotaRaiz);
  server.on("/api", rotaApi);
  server.on("/led", rotaLed);
  server.onNotFound([]() { server.send(404, "text/plain", "nao encontrado"); });

  server.begin();
  Serial.println("servidor no ar");
}

// ============================================================ loop
void loop() {
  server.handleClient();     // SEM esta linha o servidor nunca responde

  // Cuidado: qualquer delay() longo aqui trava o servidor junto.
  // Se o projeto tiver trabalho pesado, ponha numa task (oficina 3).
}

// ============================================================ rotas
void rotaRaiz() {
  server.send_P(200, "text/html", PAGINA);   // send_P: lê direto da flash
}

void rotaApi() {
  // JSON montado na mão: para três campos não compensa o ArduinoJson.
  String json = "{";
  json += "\"sensor\":" + String(analogRead(PINO_SENSOR)) + ",";
  json += "\"led\":"    + String(ledLigado ? "true" : "false") + ",";
  json += "\"uptime\":" + String(millis() / 1000);
  json += "}";

  server.send(200, "application/json", json);
}

void rotaLed() {
  if (server.hasArg("estado")) {
    ledLigado = (server.arg("estado") == "1");
    digitalWrite(PINO_LED, ledLigado ? HIGH : LOW);
    Serial.printf("LED -> %s\n", ledLigado ? "ligado" : "desligado");
  }
  server.send(200, "text/plain", ledLigado ? "1" : "0");
}

/*
 * Se a página não abrir:
 *   1. O celular está na MESMA rede? (não nos dados móveis)
 *   2. Você digitou http:// e não https://?
 *   3. O IP é o que apareceu no monitor serial?
 *   4. O .local não funciona no Android e, no Windows, só com o Bonjour.
 *      Na dúvida, use o IP.
 */
