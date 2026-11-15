#include <Arduino.h>

/*
 * Oficina de ESP32 — PETECO / UTFPR
 * 05. ESP-NOW, BLE e integração — o nó sensor (partes B, C e D)
 *
 * O que faz
 *   publica a leitura do sensor para o GATEWAY a cada 2 s
 *   escuta comandos de broadcast do gateway e acende o LED quando
 *     o comando traz o número desta bancada
 *   manda um pulso direto para a BANCADA VIZINHA quando o botão é
 *     pressionado (parte D: sem roteador, sem broker, sem IP)
 *
 * ANTES DE GRAVAR, troque:
 *   MINHA_BANCADA, MAC_GATEWAY e MAC_VIZINHA
 *
 * Montagem
 *   Sensor : LDR ou potenciômetro no GPIO 34 (ADC1)
 *   LED    : GPIO 13 -> resistor 220R -> LED -> GND
 *   Botão  : GPIO 4 -> botão -> GND  (usa pull-up interno)
 *
 * ATENÇÃO às assinaturas dos callbacks, que mudaram no core 3.x:
 *   recepção: recebe esp_now_recv_info_t* desde o 3.0
 *   envio:    recebe esp_now_send_info_t* desde o 3.3 (ESP-IDF 5.5)
 * Tutorial com (const uint8_t *mac, ...) é de versão antiga e dá erro de
 * "invalid conversion". O #if lá embaixo compila nas duas versões.
 */

#include <WiFi.h>
#include <esp_now.h>

// ----------------------------------------------------------- configuração
const uint8_t MINHA_BANCADA = 1;                 // <<< TROQUE

uint8_t MAC_GATEWAY[] = {0xA0, 0xB7, 0x65, 0x00, 0x00, 0x00};   // <<< TROQUE
uint8_t MAC_VIZINHA[] = {0xA0, 0xB7, 0x65, 0x11, 0x11, 0x11};   // <<< TROQUE
uint8_t MAC_BROADCAST[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

const int PINO_SENSOR = 34;
const int PINO_LED    = 13;
const int PINO_BOTAO  = 4;

const unsigned long INTERVALO_ENVIO_MS = 2000;
const unsigned long DEBOUNCE_MS        = 250;

// ----------------------------------------------------------- protocolo
// A MESMA struct precisa estar declarada nos dois lados, byte a byte.
// Se um lado mudar um campo, o outro passa a ler lixo.
enum TipoMsg : uint8_t { LEITURA = 1, COMANDO_LED = 2, PULSO = 3 };

struct Pacote {
  uint8_t  tipo;
  uint8_t  bancada;      // quem mandou, ou a quem o comando se destina
  float    valor;
  uint32_t contador;
};

// Limite do ESP-NOW: 250 bytes por pacote. Esta struct tem 12.
static_assert(sizeof(Pacote) <= 250, "pacote grande demais para o ESP-NOW");

// ----------------------------------------------------------- estado
Pacote aEnviar;
uint32_t contadorEnvios = 0;

volatile bool botaoPressionado = false;
volatile unsigned long ultimaBorda = 0;

// O callback de recepção só anota o pulso; quem pisca é o loop().
volatile uint8_t pulsoDaBancada = 0;     // 0 = nenhum pulso pendente

// Protótipos: a Arduino IDE os gera sozinha, o PlatformIO não.
void cadastraPar(uint8_t *mac);
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 5, 0)
void aoEnviar(const esp_now_send_info_t *info, esp_now_send_status_t status);
#else
void aoEnviar(const uint8_t *mac, esp_now_send_status_t status);
#endif
void aoReceber(const esp_now_recv_info_t *info, const uint8_t *dados, int len);
void enviaLeitura();
void trataBotao();
void trataPulso();

// ----------------------------------------------------------- ISR do botão
void IRAM_ATTR aoPressionarBotao() {
  unsigned long agora = millis();
  if (agora - ultimaBorda > DEBOUNCE_MS) {
    botaoPressionado = true;
    ultimaBorda = agora;
  }
}

// =============================================================== setup
void setup() {
  Serial.begin(115200);
  delay(500);

  pinMode(PINO_LED, OUTPUT);
  pinMode(PINO_BOTAO, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(PINO_BOTAO), aoPressionarBotao, FALLING);

  // ESP-NOW exige o rádio em modo estação, mas NÃO exige conectar a nada.
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  Serial.printf("\nbancada %u  |  meu MAC: %s  |  canal %d\n",
                MINHA_BANCADA, WiFi.macAddress().c_str(), WiFi.channel());

  if (esp_now_init() != ESP_OK) {
    Serial.println("ERRO: esp_now_init falhou");
    ESP.restart();
  }

  esp_now_register_send_cb(aoEnviar);
  esp_now_register_recv_cb(aoReceber);

  // Só quem ENVIA precisa cadastrar o destino como par.
  cadastraPar(MAC_GATEWAY);
  cadastraPar(MAC_VIZINHA);
  cadastraPar(MAC_BROADCAST);

  Serial.println("pronto");
}

// =============================================================== loop
void loop() {
  enviaLeitura();
  trataBotao();
  trataPulso();
}

// =============================================================== envio
void enviaLeitura() {
  static unsigned long ultima = 0;
  if (millis() - ultima < INTERVALO_ENVIO_MS) return;
  ultima = millis();

  aEnviar.tipo     = LEITURA;
  aEnviar.bancada  = MINHA_BANCADA;
  aEnviar.valor    = analogRead(PINO_SENSOR);
  aEnviar.contador = ++contadorEnvios;

  esp_err_t r = esp_now_send(MAC_GATEWAY, (uint8_t *)&aEnviar, sizeof(aEnviar));

  // ESP_OK aqui significa só que ENTROU NA FILA de transmissão.
  // Quem diz se chegou é o callback aoEnviar().
  if (r != ESP_OK) Serial.printf("falha ao enfileirar: %d\n", r);
}

void trataBotao() {
  if (!botaoPressionado) return;
  botaoPressionado = false;              // a ISR só levanta a flag

  Pacote p = { PULSO, MINHA_BANCADA, 0, 0 };
  esp_now_send(MAC_VIZINHA, (uint8_t *)&p, sizeof(p));
  Serial.println("-> pulso para a bancada vizinha");
}

void trataPulso() {
  if (pulsoDaBancada == 0) return;
  uint8_t de = pulsoDaBancada;
  pulsoDaBancada = 0;

  // Pisca aqui, no loop(), e não no callback: lá dentro um delay()
  // travaria a task do Wi-Fi, que também entrega os outros pacotes.
  Serial.printf("<- pulso da bancada %u\n", de);
  for (int i = 0; i < 6; i++) {
    digitalWrite(PINO_LED, !digitalRead(PINO_LED));
    delay(80);
  }
}

// =============================================================== callbacks
// Callback de envio: a assinatura mudou no core 3.3 (ESP-IDF 5.5). Antes
// recebia o MAC direto; agora recebe uma struct com o destino em des_addr.
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 5, 0)
void aoEnviar(const esp_now_send_info_t *info, esp_now_send_status_t status) {
  const uint8_t *mac = info->des_addr;
#else
void aoEnviar(const uint8_t *mac, esp_now_send_status_t status) {
#endif
  if (status != ESP_NOW_SEND_SUCCESS) {
    Serial.printf("sem confirmacao de %02X:%02X "
                  "(fora de alcance, desligada ou canal diferente)\n",
                  mac[4], mac[5]);
  }
}

// Assinatura do core 3.x: esp_now_recv_info_t em vez de const uint8_t*.
// Roda na task do Wi-Fi, como os eventos da oficina 4: nada de delay() aqui.
void aoReceber(const esp_now_recv_info_t *info, const uint8_t *dados, int len) {
  if (len != sizeof(Pacote)) return;     // não é do nosso protocolo

  Pacote p;
  memcpy(&p, dados, sizeof(p));

  switch (p.tipo) {
    case COMANDO_LED:
      // O gateway manda em broadcast: todo mundo recebe, cada um filtra.
      if (p.bancada == MINHA_BANCADA || p.bancada == 0) {   // 0 = todas
        digitalWrite(PINO_LED, p.valor > 0.5 ? HIGH : LOW);
        Serial.printf("<- comando do gateway: LED %s\n",
                      p.valor > 0.5 ? "ligado" : "desligado");
      }
      break;

    case PULSO:
      // Pulso direto da bancada vizinha, sem passar por ninguém.
      // Só anota: o trataPulso(), no loop(), faz o LED piscar.
      pulsoDaBancada = p.bancada;
      break;

    case LEITURA:
      Serial.printf("<- leitura da bancada %u: %.0f\n", p.bancada, p.valor);
      break;
  }
}

// =============================================================== auxiliares
void cadastraPar(uint8_t *mac) {
  esp_now_peer_info_t par = {};          // zera a struct inteira: importa
  memcpy(par.peer_addr, mac, 6);
  par.channel = 0;                       // 0 = usa o canal atual do rádio
  par.encrypt = false;                   // broadcast NÃO aceita criptografia

  if (esp_now_add_peer(&par) != ESP_OK) {
    Serial.printf("falha ao cadastrar %02X:%02X\n", mac[4], mac[5]);
  }
}

/*
 * NÃO CHEGA NADA? O canal é a causa mais provável.
 *
 * ESP-NOW e Wi-Fi dividem o mesmo rádio, e o rádio escuta um canal por vez.
 * Uma placa desconectada fica no canal 1; uma conectada fica no canal do
 * roteador. Se não baterem, as mensagens somem sem nenhum erro.
 *
 * Se o GATEWAY estiver conectado ao Wi-Fi, descubra o canal dele e fixe
 * o mesmo aqui, logo depois do WiFi.mode():
 *
 *   #include <esp_wifi.h>
 *   esp_wifi_set_channel(CANAL_DO_GATEWAY, WIFI_SECOND_CHAN_NONE);
 *
 * Outras causas: MAC digitado errado, struct diferente entre os dois lados,
 * ou esquecer o esp_now_add_peer no lado que envia.
 */
