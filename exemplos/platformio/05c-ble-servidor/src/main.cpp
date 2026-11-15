#include <Arduino.h>

/*
 * Oficina de ESP32 — PETECO / UTFPR
 * 05. ESP-NOW, BLE e integração — servidor BLE com NimBLE
 *
 * A placa publica um serviço GATT com duas características:
 *   temperatura  READ + NOTIFY   o celular lê e recebe avisos
 *   LED          READ + WRITE    o celular manda "1" ou "0"
 *
 * Biblioteca: NimBLE-Arduino, de h2zero (Library Manager).
 *
 * ESTE CÓDIGO É PARA A VERSÃO 2.x DA NIMBLE. Mudanças em relação à 1.x:
 *   onWrite/onConnect agora recebem também um NimBLEConnInfo&
 *   setScanResponse  virou  enableScanResponse
 *   o nome NÃO é anunciado sozinho: chame setName()
 *   o anúncio NÃO recomeça sozinho: chame advertiseOnDisconnect(true)
 *   setPower recebe dBm (9) e não mais o enum ESP_PWR_LVL_P9
 *
 * Como testar: app nRF Connect no celular. Procure por "PETECO-b1",
 * conecte, abra o serviço e use os botões de ler, escrever e notificar.
 *
 * Montagem
 *   Sensor : LDR ou potenciômetro no GPIO 34 (ADC1)
 *   LED    : GPIO 13 -> resistor 220R -> LED -> GND
 */

#include <NimBLEDevice.h>

// ----------------------------------------------------------- configuração
#define NOME_BLE "PETECO-b1"               // <<< TROQUE pelo da sua bancada

// 181A / 2A6E são UUIDs padronizados pela Bluetooth SIG: qualquer app
// sabe que é "Environmental Sensing" e "Temperature".
#define UUID_SERVICO "181A"
#define UUID_TEMP    "2A6E"
// Para algo que não tem padrão, gere um UUID de 128 bits (uuidgen).
#define UUID_LED     "6b2f0001-1f3c-4d1a-9e55-21a7c0ffee01"

const int PINO_SENSOR = 34;
const int PINO_LED    = 13;

// ----------------------------------------------------------- estado
NimBLECharacteristic *caracTemp = nullptr;
NimBLECharacteristic *caracLed  = nullptr;
volatile bool conectado = false;       // escrita pela task do BLE

// ----------------------------------------------------------- callbacks
// ATENÇÃO: na NimBLE 2.x estes métodos recebem NimBLEConnInfo&.
// Sem esse parâmetro, o override não casa e o callback nunca é chamado.
class CallbacksServidor : public NimBLEServerCallbacks {
  void onConnect(NimBLEServer *s, NimBLEConnInfo &info) override {
    conectado = true;
    Serial.println("celular conectado");
  }
  void onDisconnect(NimBLEServer *s, NimBLEConnInfo &info, int motivo) override {
    conectado = false;
    Serial.printf("celular desconectado (motivo %d)\n", motivo);
    // Com advertiseOnDisconnect(true) a própria NimBLE volta a anunciar.
  }
};

class CallbacksLed : public NimBLECharacteristicCallbacks {
  void onWrite(NimBLECharacteristic *c, NimBLEConnInfo &info) override {
    std::string valor = c->getValue();
    if (valor.empty()) return;

    bool ligar = (valor[0] == '1' || valor[0] == 1);
    digitalWrite(PINO_LED, ligar ? HIGH : LOW);
    Serial.printf("celular escreveu: LED %s\n", ligar ? "ligado" : "desligado");
  }
};

// =============================================================== setup
void setup() {
  Serial.begin(115200);
  delay(500);

  pinMode(PINO_LED, OUTPUT);

  NimBLEDevice::init(NOME_BLE);
  NimBLEDevice::setPower(9);               // 2.x: em dBm, não mais o enum

  NimBLEServer *servidor = NimBLEDevice::createServer();
  servidor->setCallbacks(new CallbacksServidor());
  servidor->advertiseOnDisconnect(true);   // 2.x: não recomeça sozinho

  NimBLEService *servico = servidor->createService(UUID_SERVICO);

  caracTemp = servico->createCharacteristic(
      UUID_TEMP, NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY);

  caracLed = servico->createCharacteristic(
      UUID_LED, NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE);
  caracLed->setCallbacks(new CallbacksLed());
  caracLed->setValue("0");

  servico->start();

  NimBLEAdvertising *anuncio = NimBLEDevice::getAdvertising();
  anuncio->setName(NOME_BLE);              // 2.x: o nome não vai sozinho
  anuncio->addServiceUUID(UUID_SERVICO);
  anuncio->enableScanResponse(true);       // 2.x: era setScanResponse
  NimBLEDevice::startAdvertising();

  Serial.printf("anunciando como \"%s\"\n", NOME_BLE);
  Serial.println("abra o nRF Connect no celular e procure por ele");
}

// =============================================================== loop
void loop() {
  static unsigned long ultima = 0;
  if (millis() - ultima < 1000) return;
  ultima = millis();

  // A característica 2A6E é padronizada: int16 em centésimos de grau.
  // Seguir o padrão faz o app mostrar "23.45 °C" já formatado.
  float graus = analogRead(PINO_SENSOR) / 100.0;   // troque pelo sensor real
  int16_t centesimos = (int16_t)(graus * 100);

  caracTemp->setValue((uint8_t *)&centesimos, 2);  // só guarda o valor

  if (conectado) {
    caracTemp->notify();                           // é isto que avisa o celular
    Serial.printf("notify: %.2f\n", graus);
  }
}

/*
 * Não aparece no celular?
 *   1. Bluetooth E localização ligados (o Android exige os dois para varrer)
 *   2. A placa já está conectada a outro celular? O BLE aceita poucos
 *      clientes, e enquanto conectado ela para de anunciar
 *   3. Confira a versão da NimBLE no Library Manager: exemplo da 1.x
 *      não compila na 2.x
 *
 * Firmware grande demais?
 *   BLE ocupa bastante flash. Se der "sketch too big", troque o
 *   Partition Scheme para um com mais espaço de aplicação
 *   (Ferramentas > Partition Scheme > "Huge APP").
 */
