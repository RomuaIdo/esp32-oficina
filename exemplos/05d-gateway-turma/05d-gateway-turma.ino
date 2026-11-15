/*
 * Oficina de ESP32 — PETECO / UTFPR
 * 05. ESP-NOW, BLE e integração — o GATEWAY da prática final
 *
 * Esta é a placa do professor. Ela fecha a série inteira:
 *
 *   ESP-NOW  recebe as leituras das 12 bancadas          (oficina 5)
 *   Wi-Fi    sobe um painel web com todas elas           (oficina 4)
 *   mutex    o callback do rádio roda em outra task      (oficina 3)
 *   millis   nada bloqueia o loop                        (oficina 1)
 *
 * Abra o IP no projetor: as bancadas aparecem sozinhas conforme ligam.
 *
 * ===================================================================
 * O CANAL É O PONTO CRÍTICO
 * ESP-NOW e Wi-Fi dividem o mesmo rádio, que escuta um canal por vez.
 * Como este gateway CONECTA ao roteador, ele fica no canal do roteador.
 * Os nós precisam estar no MESMO canal, ou nada chega.
 *
 * Duas saídas:
 *   a) deixe MODO_AP = false e diga aos alunos o canal que aparecer no
 *      serial, para eles fixarem com esp_wifi_set_channel()
 *   b) deixe MODO_AP = true: o gateway vira o ponto de acesso e você
 *      escolhe o canal. Mais simples e sem depender do roteador.
 *
 * No modo AP o rádio fica em WIFI_AP_STA, e não só WIFI_AP: os nós
 * mandam para o MAC de ESTAÇÃO (o que o 05a imprime), e essa interface
 * precisa estar ligada para receber. O ponto de acesso tem outro MAC.
 * ===================================================================
 */

#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <WebServer.h>

// ----------------------------------------------------------- configuração
const bool MODO_AP = true;            // true = o gateway cria a própria rede

const char *AP_SSID  = "PETECO-Gateway";
const char *AP_SENHA = "oficina5esp32";
const int   AP_CANAL = 1;             // os nós precisam estar neste canal

const char *STA_SSID  = "PETECO-Oficina";    // usados quando MODO_AP = false
const char *STA_SENHA = "esp32peteco";

const int MAX_BANCADAS = 16;
const unsigned long TIMEOUT_OFFLINE_MS = 10000;   // sem pacote = offline

// ----------------------------------------------------------- protocolo
// A MESMA struct do 05b, byte a byte.
enum TipoMsg : uint8_t { LEITURA = 1, COMANDO_LED = 2, PULSO = 3 };

struct Pacote {
  uint8_t  tipo;
  uint8_t  bancada;
  float    valor;
  uint32_t contador;
};

uint8_t MAC_BROADCAST[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

// ----------------------------------------------------------- estado
struct Bancada {
  bool     vista;
  uint8_t  mac[6];
  float    valor;
  uint32_t pacotes;
  uint32_t perdidos;
  unsigned long ultimoMs;
};

Bancada bancadas[MAX_BANCADAS];
SemaphoreHandle_t mutexDados;
WebServer server(80);

// Protótipos: a Arduino IDE os gera sozinha, o PlatformIO não.
void aoReceber(const esp_now_recv_info_t *info, const uint8_t *dados, int len);
void rotaRaiz();
void rotaApi();
void rotaComando();
String macParaTexto(const uint8_t *mac);

const char PAGINA[] PROGMEM = R"HTML(
<!DOCTYPE html><html lang="pt-BR">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width,initial-scale=1">
  <title>PETECO - Rede da turma</title>
  <style>
    body{font-family:system-ui,sans-serif;background:#14161a;color:#fff;
         margin:0;padding:28px}
    h1{color:#f5c400;font-size:26px;margin:0 0 4px;text-align:center}
    .sub{color:#8b93a1;font-size:13px;text-align:center;margin-bottom:24px}
    .grade{display:grid;gap:14px;
           grid-template-columns:repeat(auto-fill,minmax(190px,1fr));
           max-width:1100px;margin:0 auto}
    .cartao{background:#1c1f26;border-radius:12px;padding:16px;
            border-left:5px solid #1f8a4c}
    .cartao.off{border-left-color:#5b6270;opacity:.45}
    .nome{color:#f5c400;font-weight:700;font-size:15px}
    .valor{font-size:38px;font-weight:800;margin:6px 0}
    .meta{color:#5b6270;font-size:11px;line-height:1.6}
    .barra{text-align:center;margin-top:28px}
    button{font-size:15px;padding:12px 26px;margin:0 6px;border:none;
           border-radius:8px;background:#f5c400;font-weight:700;cursor:pointer}
    .off-btn{background:#5b6270;color:#fff}
    .vazio{color:#5b6270;text-align:center;padding:50px;font-size:15px}
  </style>
</head>
<body>
  <h1>Rede da turma</h1>
  <div class="sub"><span id="online">0</span> bancada(s) no ar &middot;
       <span id="total">0</span> pacote(s) recebido(s)</div>

  <div class="grade" id="grade"></div>
  <div class="vazio" id="vazio">Nenhuma bancada ainda. Liguem as placas!</div>

  <div class="barra">
    <button onclick="cmd(0,1)">ACENDER TODOS</button>
    <button class="off-btn" onclick="cmd(0,0)">APAGAR TODOS</button>
  </div>

<script>
async function cmd(bancada, v){ await fetch(`/cmd?b=${bancada}&v=${v}`); }

async function atualiza(){
  const d = await (await fetch('/api')).json();
  const g = document.getElementById('grade');

  document.getElementById('vazio').style.display = d.bancadas.length ? 'none' : 'block';
  document.getElementById('online').textContent = d.online;
  document.getElementById('total').textContent  = d.total;

  g.innerHTML = d.bancadas.map(b => `
    <div class="cartao ${b.online ? '' : 'off'}">
      <div class="nome">Bancada ${b.id}</div>
      <div class="valor">${Math.round(b.valor)}</div>
      <div class="meta">${b.mac}<br>${b.pacotes} pacotes
        ${b.perdidos ? ' &middot; ' + b.perdidos + ' perdidos' : ''}<br>
        ha ${b.ha}s</div>
    </div>`).join('');
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

  mutexDados = xSemaphoreCreateMutex();
  for (int i = 0; i < MAX_BANCADAS; i++) bancadas[i].vista = false;

  if (MODO_AP) {
    WiFi.mode(WIFI_AP_STA);             // AP para o painel + STA para o ESP-NOW
    WiFi.softAP(AP_SSID, AP_SENHA, AP_CANAL);
    Serial.printf("\nAP \"%s\" no canal %d\n", AP_SSID, AP_CANAL);
    Serial.printf("painel: http://%s\n", WiFi.softAPIP().toString().c_str());
  } else {
    WiFi.mode(WIFI_STA);
    WiFi.begin(STA_SSID, STA_SENHA);
    Serial.print("conectando");
    while (WiFi.status() != WL_CONNECTED) { delay(300); Serial.print("."); }
    Serial.printf("\npainel: http://%s\n", WiFi.localIP().toString().c_str());
  }

  Serial.printf("MAC do gateway (para MAC_GATEWAY no 05b): %s\n",
                WiFi.macAddress().c_str());
  Serial.printf(">>> DIGA AOS ALUNOS: canal %d <<<\n\n", WiFi.channel());

  if (esp_now_init() != ESP_OK) {
    Serial.println("ERRO: esp_now_init falhou");
    ESP.restart();
  }
  esp_now_register_recv_cb(aoReceber);

  esp_now_peer_info_t par = {};
  memcpy(par.peer_addr, MAC_BROADCAST, 6);
  par.channel = 0;
  par.encrypt = false;
  esp_now_add_peer(&par);

  server.on("/",    rotaRaiz);
  server.on("/api", rotaApi);
  server.on("/cmd", rotaComando);
  server.begin();
}

void loop() {
  server.handleClient();
}

// =============================================================== ESP-NOW
// Assinatura do core 3.x: esp_now_recv_info_t, com o remetente em src_addr.
// Roda na task do Wi-Fi, não no loop(): por isso a tabela de bancadas,
// que o loop() lê para montar o /api, fica protegida pelo mutex.
void aoReceber(const esp_now_recv_info_t *info, const uint8_t *dados, int len) {
  if (len != sizeof(Pacote)) return;

  Pacote p;
  memcpy(&p, dados, sizeof(p));
  if (p.tipo != LEITURA || p.bancada >= MAX_BANCADAS) return;

  if (xSemaphoreTake(mutexDados, pdMS_TO_TICKS(50)) != pdTRUE) return;

  Bancada &b = bancadas[p.bancada];

  // Buraco na sequência do contador = pacote perdido no caminho.
  // É a forma honesta de mostrar a qualidade do enlace no painel.
  if (b.vista && p.contador > b.pacotes + 1) {
    b.perdidos += (p.contador - b.pacotes - 1);
  }

  b.vista    = true;
  b.valor    = p.valor;
  b.pacotes  = p.contador;
  b.ultimoMs = millis();
  memcpy(b.mac, info->src_addr, 6);

  xSemaphoreGive(mutexDados);
}

// =============================================================== rotas
void rotaRaiz() {
  server.send_P(200, "text/html", PAGINA);
}

void rotaApi() {
  String json = "{\"bancadas\":[";
  int online = 0;
  uint32_t total = 0;
  bool primeira = true;

  if (xSemaphoreTake(mutexDados, pdMS_TO_TICKS(100)) != pdTRUE) {
    server.send(503, "text/plain", "ocupado");
    return;
  }

  for (int i = 0; i < MAX_BANCADAS; i++) {
    if (!bancadas[i].vista) continue;

    unsigned long ha = (millis() - bancadas[i].ultimoMs) / 1000;
    bool viva = (millis() - bancadas[i].ultimoMs) < TIMEOUT_OFFLINE_MS;
    if (viva) online++;
    total += bancadas[i].pacotes;

    if (!primeira) json += ",";
    primeira = false;

    json += "{\"id\":"        + String(i);
    json += ",\"valor\":"     + String(bancadas[i].valor, 0);
    json += ",\"pacotes\":"   + String(bancadas[i].pacotes);
    json += ",\"perdidos\":"  + String(bancadas[i].perdidos);
    json += ",\"ha\":"        + String(ha);
    json += ",\"online\":"    + String(viva ? "true" : "false");
    json += ",\"mac\":\""     + macParaTexto(bancadas[i].mac) + "\"}";
  }

  xSemaphoreGive(mutexDados);

  json += "],\"online\":" + String(online) + ",\"total\":" + String(total) + "}";
  server.send(200, "application/json", json);
}

void rotaComando() {
  uint8_t bancada = server.hasArg("b") ? server.arg("b").toInt() : 0;
  float   v       = server.hasArg("v") ? server.arg("v").toFloat() : 0;

  // bancada 0 = todas. O broadcast chega a todo mundo, e cada nó filtra.
  Pacote p = { COMANDO_LED, bancada, v, 0 };
  esp_now_send(MAC_BROADCAST, (uint8_t *)&p, sizeof(p));

  Serial.printf("-> comando: bancada %u, LED %s\n",
                bancada, v > 0.5 ? "ligado" : "desligado");
  server.send(200, "text/plain", "ok");
}

String macParaTexto(const uint8_t *mac) {
  char buf[18];
  snprintf(buf, sizeof(buf), "%02X:%02X:%02X:%02X:%02X:%02X",
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  return String(buf);
}
