#include <Arduino.h>

/*
 * Oficina de ESP32 — PETECO / UTFPR
 * 04. Wi-Fi e IoT — solução da prática (partes A a D + desafio)
 *
 * Junta tudo da oficina:
 *   conexão por eventos, com reconexão automática    (parte A)
 *   servidor web com painel que atualiza sozinho     (partes B e C)
 *   MQTT publicando leituras e recebendo comandos    (parte D)
 *   assinatura do tópico da bancada vizinha          (desafio)
 *   NTP para carimbar a hora
 *
 * Arquitetura, aproveitando a oficina 3: o MQTT roda numa TASK própria,
 * para que uma reconexão lenta não trave o servidor web. As duas pontas
 * conversam por variáveis protegidas com mutex.
 *
 * Montagem
 *   Sensor : LDR ou potenciômetro no GPIO 34 (ADC1)
 *   LED    : GPIO 13 -> resistor 220R -> LED -> GND
 *
 * Tópicos (com BANCADA = "b1")
 *   publica  peteco/b1/sensor        leitura + hora, a cada 5 s (retain)
 *   publica  peteco/b1/status        online / offline (last will)
 *   publica  peteco/b1/led/estado    o LED mudou, pela página ou por MQTT
 *   assina   peteco/b1/led           comando: "1" ou "0"
 *   assina   peteco/b2/sensor        a bancada vizinha (desafio)
 *
 * TROQUE: rede, broker, BANCADA e VIZINHA.
 */

#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <PubSubClient.h>
#include <time.h>

// ----------------------------------------------------------- configuração
const char *SSID    = "PETECO-Oficina";
const char *SENHA   = "esp32peteco";
const char *BROKER  = "192.168.1.100";
const char *BANCADA = "b1";               // <<< TROQUE
const char *VIZINHA = "b2";               // <<< TROQUE (desafio)
const char *NOME_MDNS = "peteco";

const int PINO_SENSOR = 34;
const int PINO_LED    = 13;

// ----------------------------------------------------------- estado
WebServer server(80);
WiFiClient wifi;
PubSubClient mqtt(wifi);

SemaphoreHandle_t mutexEstado;            // protege as variáveis abaixo

int   sensorAtual   = 0;
bool  ledLigado     = false;
int   sensorVizinha = -1;                 // -1 = ainda não chegou nada
bool  mqttOk        = false;

// O PubSubClient NÃO é thread-safe: só a tarefaMqtt pode tocar no objeto
// mqtt. Quando o servidor web (que roda na task do loop) precisa publicar
// algo, ele levanta esta flag e a tarefaMqtt publica por ele.
bool  pendenteAvisarLed = false;

String topicoSensor, topicoLed, topicoLedEstado, topicoStatus, topicoVizinha,
       clienteId;

// Protótipos: a Arduino IDE os gera sozinha, o PlatformIO não.
void conectaWiFi();
void tarefaMqtt(void *p);
void garanteConexaoMqtt();
void aoReceber(char *topico, byte *dados, unsigned int tamanho);
void rotaRaiz();
void rotaApi();
void rotaLed();
String horaAtual();

const char PAGINA[] PROGMEM = R"HTML(
<!DOCTYPE html><html lang="pt-BR">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width,initial-scale=1">
  <title>PETECO - Oficina 4</title>
  <style>
    body{font-family:system-ui,sans-serif;background:#14161a;color:#fff;
         margin:0;padding:24px;text-align:center}
    h1{color:#f5c400;font-size:20px;margin:0 0 20px}
    .cartao{background:#1c1f26;border-radius:12px;padding:18px;margin:12px 0}
    .valor{font-size:56px;font-weight:800;margin:6px 0}
    .rotulo{color:#8b93a1;font-size:12px;text-transform:uppercase;
            letter-spacing:.1em}
    .vizinha .valor{font-size:34px;color:#8b93a1}
    button{font-size:17px;padding:13px 30px;margin-top:8px;border:none;
           border-radius:8px;background:#f5c400;font-weight:700;cursor:pointer}
    .off{background:#5b6270;color:#fff}
    .rodape{color:#5b6270;font-size:12px;margin-top:22px;line-height:1.7}
    .pt{display:inline-block;width:8px;height:8px;border-radius:50%;
        background:#c8102e;margin-right:6px}
    .pt.on{background:#1f8a4c}
  </style>
</head>
<body>
  <h1>PETECO &middot; bancada <span id="bancada">--</span></h1>

  <div class="cartao">
    <div class="rotulo">Meu sensor</div>
    <div class="valor" id="valor">--</div>
    <button id="btn" onclick="alterna()">carregando...</button>
  </div>

  <div class="cartao vizinha">
    <div class="rotulo">Bancada vizinha (<span id="viz">--</span>)</div>
    <div class="valor" id="valorViz">--</div>
  </div>

  <div class="rodape">
    <span class="pt" id="ptMqtt"></span>MQTT<br>
    <span id="hora">--</span><br>
    ligado ha <span id="uptime">0</span> s
  </div>

<script>
let ligado = false;

async function atualiza(){
  try{
    const d = await (await fetch('/api')).json();
    document.getElementById('valor').textContent    = d.sensor;
    document.getElementById('valorViz').textContent = d.vizinha < 0 ? '--' : d.vizinha;
    document.getElementById('bancada').textContent  = d.bancada;
    document.getElementById('viz').textContent      = d.nomeVizinha;
    document.getElementById('uptime').textContent   = d.uptime;
    document.getElementById('hora').textContent     = d.hora;
    document.getElementById('ptMqtt').className     = 'pt' + (d.mqtt ? ' on' : '');
    ligado = d.led;
    pintaBotao();
  }catch(e){
    document.getElementById('valor').textContent = '--';
  }
}
function pintaBotao(){
  const b = document.getElementById('btn');
  b.textContent = ligado ? 'DESLIGAR LED' : 'LIGAR LED';
  b.className   = ligado ? '' : 'off';
}
async function alterna(){
  await fetch('/led?estado=' + (ligado ? 0 : 1));
  atualiza();
}
setInterval(atualiza, 1000);
atualiza();
</script>
</body></html>
)HTML";

// =============================================================== setup
void setup() {
  Serial.begin(115200);
  delay(500);

  pinMode(PINO_LED, OUTPUT);

  topicoSensor   = String("peteco/") + BANCADA + "/sensor";
  topicoLed      = String("peteco/") + BANCADA + "/led";
  topicoLedEstado = topicoLed + "/estado";
  topicoStatus   = String("peteco/") + BANCADA + "/status";
  topicoVizinha  = String("peteco/") + VIZINHA + "/sensor";
  clienteId      = String("esp32-") + BANCADA + "-" +
                   String((uint32_t)ESP.getEfuseMac(), HEX);

  mutexEstado = xSemaphoreCreateMutex();

  conectaWiFi();

  // Hora certa para carimbar as leituras. Fuso de Curitiba: UTC-3.
  configTime(-3 * 3600, 0, "pool.ntp.org", "a.st1.ntp.br");

  if (MDNS.begin(NOME_MDNS)) MDNS.addService("http", "tcp", 80);

  server.on("/",    rotaRaiz);
  server.on("/api", rotaApi);
  server.on("/led", rotaLed);
  server.begin();

  mqtt.setServer(BROKER, 1883);
  mqtt.setCallback(aoReceber);
  mqtt.setBufferSize(512);

  // O MQTT vai para uma task própria: assim uma reconexão lenta
  // (que pode levar segundos) não trava o servidor web.
  xTaskCreatePinnedToCore(tarefaMqtt, "mqtt", 8192, NULL, 1, NULL, 1);

  Serial.printf("\n  painel:  http://%s\n", WiFi.localIP().toString().c_str());
  Serial.printf("  ou:      http://%s.local\n\n", NOME_MDNS);
}

// =============================================================== loop
void loop() {
  server.handleClient();

  // Lê o sensor a cada 500 ms e guarda sob mutex.
  static unsigned long ultima = 0;
  if (millis() - ultima >= 500) {
    ultima = millis();
    int v = analogRead(PINO_SENSOR);

    if (xSemaphoreTake(mutexEstado, pdMS_TO_TICKS(50))) {
      sensorAtual = v;
      xSemaphoreGive(mutexEstado);
    }
  }
}

// =============================================================== MQTT
void tarefaMqtt(void *p) {
  for (;;) {
    if (WiFi.status() == WL_CONNECTED) {
      if (!mqtt.connected()) garanteConexaoMqtt();
      mqtt.loop();

      // O servidor web pediu para avisar que o LED mudou? De quebra,
      // atualiza o indicador de MQTT do painel.
      bool avisar = false, estadoLed = false;
      if (xSemaphoreTake(mutexEstado, pdMS_TO_TICKS(50))) {
        mqttOk = mqtt.connected();
        avisar = pendenteAvisarLed;
        estadoLed = ledLigado;
        pendenteAvisarLed = false;
        xSemaphoreGive(mutexEstado);
      }
      // O estado vai num tópico SEPARADO do comando. Se fosse no próprio
      // peteco/b1/led, a placa receberia de volta o que acabou de publicar.
      if (avisar && mqtt.connected()) {
        mqtt.publish(topicoLedEstado.c_str(), estadoLed ? "1" : "0", true);
      }

      // Publicação periódica da leitura.
      static unsigned long ultima = 0;
      if (millis() - ultima >= 5000) {
        ultima = millis();

        int v;
        if (xSemaphoreTake(mutexEstado, pdMS_TO_TICKS(50))) {
          v = sensorAtual;
          xSemaphoreGive(mutexEstado);

          char json[120];
          snprintf(json, sizeof(json), "{\"valor\":%d,\"hora\":\"%s\"}",
                   v, horaAtual().c_str());
          mqtt.publish(topicoSensor.c_str(), json, true);
        }
      }
    } else if (xSemaphoreTake(mutexEstado, pdMS_TO_TICKS(50))) {
      mqttOk = false;                  // sem Wi-Fi, sem MQTT
      xSemaphoreGive(mutexEstado);
    }
    vTaskDelay(pdMS_TO_TICKS(50));     // cede a CPU: nunca gire em vazio
  }
}

void garanteConexaoMqtt() {
  bool ok = mqtt.connect(clienteId.c_str(), nullptr, nullptr,
                         topicoStatus.c_str(), 1, true, "offline");

  if (xSemaphoreTake(mutexEstado, pdMS_TO_TICKS(50))) {
    mqttOk = ok;
    xSemaphoreGive(mutexEstado);
  }

  if (ok) {
    mqtt.publish(topicoStatus.c_str(), "online", true);
    mqtt.subscribe(topicoLed.c_str());        // reassine a cada reconexão
    mqtt.subscribe(topicoVizinha.c_str());    // desafio: a bancada vizinha
    Serial.println("MQTT conectado");
  } else {
    Serial.printf("MQTT falhou (estado %d)\n", mqtt.state());
    vTaskDelay(pdMS_TO_TICKS(2000));
  }
}

void aoReceber(char *topico, byte *dados, unsigned int tamanho) {
  String msg;
  for (unsigned int i = 0; i < tamanho; i++) msg += (char)dados[i];
  String t = String(topico);

  if (t == topicoLed) {
    bool ligar = (msg == "1" || msg == "true" || msg == "on");
    digitalWrite(PINO_LED, ligar ? HIGH : LOW);
    if (xSemaphoreTake(mutexEstado, pdMS_TO_TICKS(50))) {
      ledLigado = ligar;
      pendenteAvisarLed = true;        // confirma em peteco/b1/led/estado
      xSemaphoreGive(mutexEstado);
    }

  } else if (t == topicoVizinha) {
    // Extrai o número do JSON {"valor":1234,...} sem usar biblioteca.
    int i = msg.indexOf("\"valor\":");
    if (i >= 0) {
      int v = msg.substring(i + 8).toInt();
      if (xSemaphoreTake(mutexEstado, pdMS_TO_TICKS(50))) {
        sensorVizinha = v;
        xSemaphoreGive(mutexEstado);
      }
    }
  }
}

// =============================================================== rotas
void rotaRaiz() {
  server.send_P(200, "text/html", PAGINA);
}

void rotaApi() {
  int v, viz; bool led, mq;

  if (xSemaphoreTake(mutexEstado, pdMS_TO_TICKS(100))) {
    v = sensorAtual; viz = sensorVizinha; led = ledLigado; mq = mqttOk;
    xSemaphoreGive(mutexEstado);
  } else {
    server.send(503, "text/plain", "ocupado");
    return;
  }

  String json = "{";
  json += "\"sensor\":"      + String(v) + ",";
  json += "\"vizinha\":"     + String(viz) + ",";
  json += "\"led\":"         + String(led ? "true" : "false") + ",";
  json += "\"mqtt\":"        + String(mq ? "true" : "false") + ",";
  json += "\"bancada\":\""   + String(BANCADA) + "\",";
  json += "\"nomeVizinha\":\"" + String(VIZINHA) + "\",";
  json += "\"hora\":\""      + horaAtual() + "\",";
  json += "\"uptime\":"      + String(millis() / 1000);
  json += "}";

  server.send(200, "application/json", json);
}

void rotaLed() {
  if (server.hasArg("estado")) {
    bool ligar = (server.arg("estado") == "1");
    digitalWrite(PINO_LED, ligar ? HIGH : LOW);

    // Esta rota roda na task do loop(). Como o PubSubClient não é
    // thread-safe, NÃO publicamos daqui: deixamos o recado e a
    // tarefaMqtt publica. É o mesmo princípio da ISR da oficina 1 —
    // quem não é dono do recurso só levanta uma flag.
    if (xSemaphoreTake(mutexEstado, pdMS_TO_TICKS(50))) {
      ledLigado = ligar;
      pendenteAvisarLed = true;
      xSemaphoreGive(mutexEstado);
    }
  }
  server.send(200, "text/plain", "ok");
}

// =============================================================== auxiliares
void conectaWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);

  WiFi.onEvent([](WiFiEvent_t e, WiFiEventInfo_t i) {
    Serial.printf("Wi-Fi caiu (motivo %d)\n", i.wifi_sta_disconnected.reason);
    WiFi.reconnect();
  }, ARDUINO_EVENT_WIFI_STA_DISCONNECTED);

  WiFi.begin(SSID, SENHA);
  Serial.print("conectando");
  while (WiFi.status() != WL_CONNECTED) { delay(300); Serial.print("."); }
  Serial.printf(" ok, IP %s\n", WiFi.localIP().toString().c_str());
}

String horaAtual() {
  struct tm t;
  if (!getLocalTime(&t, 100)) return "--:--:--";   // ainda não sincronizou

  char buf[16];
  strftime(buf, sizeof(buf), "%H:%M:%S", &t);
  return String(buf);
}
