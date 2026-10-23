/*
 * Oficina de ESP32 — PETECO / UTFPR
 * 04. Wi-Fi e IoT — MQTT: publicar leituras e receber comandos
 *
 * Biblioteca: PubSubClient, de Nick O'Leary (Library Manager)
 *
 * Tópicos usados
 *   publica  peteco/<bancada>/sensor        a leitura, a cada 5 s
 *   publica  peteco/<bancada>/status        online / offline
 *   assina   peteco/<bancada>/led           comando: "1" ou "0"
 *
 * Para acompanhar pelo terminal, no computador do broker:
 *   mosquitto_sub -h localhost -t 'peteco/#' -v
 *   mosquitto_pub -h localhost -t 'peteco/b1/led' -m '1'
 *
 * TROQUE as três constantes abaixo: rede, broker e NOME DA SUA BANCADA.
 * Duas bancadas publicando no mesmo tópico vão se atrapalhar.
 */

#include <WiFi.h>
#include <PubSubClient.h>

const char *SSID    = "PETECO-Oficina";
const char *SENHA   = "esp32peteco";
const char *BROKER  = "192.168.1.100";     // IP do notebook com o Mosquitto
const int   PORTA   = 1883;
const char *BANCADA = "b1";                // <<< TROQUE PELO SEU

const int PINO_SENSOR = 34;
const int PINO_LED    = 13;

const unsigned long INTERVALO_PUBLICACAO_MS = 5000;

WiFiClient wifi;
PubSubClient mqtt(wifi);

// Montados no setup a partir do nome da bancada.
String topicoSensor, topicoStatus, topicoLed, clienteId;

// Protótipos: a Arduino IDE os gera sozinha, o PlatformIO não.
void conectaWiFi();
void garanteConexaoMqtt();
void aoReceber(char *topico, byte *dados, unsigned int tamanho);
void publicaLeitura();

// ============================================================ setup
void setup() {
  Serial.begin(115200);
  delay(500);

  pinMode(PINO_LED, OUTPUT);

  topicoSensor = String("peteco/") + BANCADA + "/sensor";
  topicoStatus = String("peteco/") + BANCADA + "/status";
  topicoLed    = String("peteco/") + BANCADA + "/led";

  // O client ID precisa ser ÚNICO no broker. Se duas placas usarem o
  // mesmo, o broker derruba a anterior — e as duas ficam reconectando
  // em laço. O MAC do chip resolve isso de vez.
  clienteId = String("esp32-") + BANCADA + "-" +
              String((uint32_t)ESP.getEfuseMac(), HEX);

  conectaWiFi();

  mqtt.setServer(BROKER, PORTA);
  mqtt.setCallback(aoReceber);
  mqtt.setBufferSize(512);        // o padrão é 256 bytes: pouco para JSON

  Serial.printf("broker ....: %s:%d\n", BROKER, PORTA);
  Serial.printf("client id .: %s\n", clienteId.c_str());
  Serial.printf("publica ...: %s\n", topicoSensor.c_str());
  Serial.printf("assina ....: %s\n", topicoLed.c_str());
}

// ============================================================ loop
void loop() {
  if (WiFi.status() != WL_CONNECTED) conectaWiFi();
  if (!mqtt.connected()) garanteConexaoMqtt();

  mqtt.loop();            // OBRIGATÓRIO: é aqui que as mensagens chegam

  publicaLeitura();
}

// ============================================================ conexões
void conectaWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(SSID, SENHA);
  Serial.print("Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) { delay(300); Serial.print("."); }
  Serial.printf(" ok, IP %s\n", WiFi.localIP().toString().c_str());
}

void garanteConexaoMqtt() {
  while (!mqtt.connected()) {
    Serial.print("conectando ao broker... ");

    // O "last will": se a placa sumir sem avisar, o broker publica
    // "offline" neste tópico sozinho. É assim que um painel sabe que
    // um nó caiu, em vez de mostrar o último valor para sempre.
    bool ok = mqtt.connect(clienteId.c_str(),
                           nullptr, nullptr,          // usuário e senha
                           topicoStatus.c_str(), 1, true, "offline");

    if (ok) {
      Serial.println("ok");
      mqtt.publish(topicoStatus.c_str(), "online", true);  // true = retain

      // As assinaturas NÃO sobrevivem à reconexão: reassine sempre aqui.
      mqtt.subscribe(topicoLed.c_str());
      Serial.printf("assinado: %s\n", topicoLed.c_str());
    } else {
      // Códigos úteis: -2 = não achou o broker (IP errado ou firewall)
      //                 5 = não autorizado
      Serial.printf("falhou (estado %d), tentando em 2 s\n", mqtt.state());
      delay(2000);
    }
  }
}

// ============================================================ mensagens
// Chamado de dentro do mqtt.loop(), ou seja: na mesma task do loop().
// Não precisa de mutex aqui, mas o callback precisa ser RÁPIDO —
// enquanto ele roda, nenhuma outra mensagem é processada.
void aoReceber(char *topico, byte *dados, unsigned int tamanho) {
  String msg;
  for (unsigned int i = 0; i < tamanho; i++) msg += (char)dados[i];

  Serial.printf("<- [%s] %s\n", topico, msg.c_str());

  if (String(topico) == topicoLed) {
    bool ligar = (msg == "1" || msg == "true" || msg == "on");
    digitalWrite(PINO_LED, ligar ? HIGH : LOW);
  }
}

void publicaLeitura() {
  static unsigned long ultima = 0;
  if (millis() - ultima < INTERVALO_PUBLICACAO_MS) return;
  ultima = millis();

  int valor = analogRead(PINO_SENSOR);

  char json[96];
  snprintf(json, sizeof(json), "{\"valor\":%d,\"uptime\":%lu}",
           valor, millis() / 1000);

  // retain = true: quem assinar depois recebe o último valor na hora,
  // em vez de esperar os próximos 5 segundos.
  mqtt.publish(topicoSensor.c_str(), json, true);

  Serial.printf("-> [%s] %s\n", topicoSensor.c_str(), json);
}

/*
 * Problemas comuns
 *
 *   conecta e cai em segundos ...... duas placas com o mesmo client ID
 *   estado -2 ...................... IP do broker errado, ou firewall do
 *                                    notebook bloqueando a porta 1883
 *   mensagem grande não chega ...... buffer de 256 bytes; use setBufferSize
 *   para de receber depois de um tempo .. mqtt.loop() não está rodando
 *   voltou mas não recebe mais ..... faltou reassinar dentro do connect()
 *
 * Para usar o broker público de teste (broker.hivemq.com, porta 1883):
 * ele é ABERTO — qualquer um lê o que você publica. Nada sensível, e
 * troque "peteco" por um prefixo bem específico para não colidir.
 */
