#include <Arduino.h>

/*
 * Oficina de ESP32 — PETECO / UTFPR
 * 03. FreeRTOS, flash e energia — solução da prática (partes A a D)
 *
 * Junta tudo da oficina, na ordem do roteiro:
 *   A  duas tasks: tarefaSensor lê, tarefaDisplay mostra
 *   B  as duas conversam por fila, sem variável global compartilhada
 *   C  cada leitura vai para /log.csv no LittleFS; o intervalo de
 *      amostragem fica no Preferences; a tela mostra as linhas do arquivo
 *   D  contador de boots em RTC_DATA_ATTR e deep sleep de 10 s por ciclo
 *   Desafio: acorda também pelo botão (ext0) e mostra a causa na tela
 *
 * A partição do LittleFS já está configurada no platformio.ini.
 * Bibliotecas: Adafruit SSD1306 e GFX, já listadas em lib_deps.
 *
 * Montagem
 *   Sensor : LDR ou potenciômetro no GPIO 34 (ADC1)
 *   OLED   : SDA no GPIO 21, SCL no GPIO 22   (opcional; sem ele só vai ao serial)
 *   Botão  : GPIO 33 -> botão -> 3V3, com 10k do GPIO 33 ao GND
 *
 * Como funciona um ciclo
 *   acorda -> coleta AMOSTRAS_POR_CICLO leituras -> grava -> dorme
 */

#include <Preferences.h>
#include <LittleFS.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ----------------------------------------------------------- configuração
const int PINO_SENSOR = 34;
const gpio_num_t PINO_ACORDA = GPIO_NUM_33;
const char *ARQUIVO_LOG = "/log.csv";

const int AMOSTRAS_POR_CICLO = 20;      // 20 amostras a 100 ms = 2 s acordado
const int PERIODO_PADRAO_MS  = 100;     // usado só até existir um no Preferences
const uint64_t SEGUNDOS_DORMINDO = 10;

// ----------------------------------------------------------- estado persistente
RTC_DATA_ATTR uint32_t boots = 0;       // sobrevive ao sono, não ao reset

Preferences prefs;
Adafruit_SSD1306 display(128, 64, &Wire, -1);
bool temDisplay = false;

// ----------------------------------------------------------- fila
struct Leitura {
  uint32_t ms;
  int      valor;
};

QueueHandle_t fila;
SemaphoreHandle_t fimDaColeta;          // avisa o setup que o ciclo acabou

int periodoAmostraMs = PERIODO_PADRAO_MS;   // lido do Preferences no setup
int linhasNoArquivo = 0;                    // contadas no boot, somadas a cada gravação
const char *motivo = "";                    // causa do despertar, para a tela

// Protótipos: a Arduino IDE os gera sozinha, o PlatformIO não.
void dorme();
int  contaLinhas();
void iniciaDisplay();
void atualizaTela(int valor, int n);
void mostra(const char *titulo, const char *linha);
const char *motivoDoDespertar();

// =============================================================== tasks
// Produtor: lê o sensor em período exato e manda para a fila.
void tarefaSensor(void *parametros) {
  TickType_t ultimoDespertar = xTaskGetTickCount();

  for (int i = 0; i < AMOSTRAS_POR_CICLO; i++) {
    Leitura L = { millis(), analogRead(PINO_SENSOR) };
    xQueueSend(fila, &L, portMAX_DELAY);     // aqui esperamos: não perder dado
    vTaskDelayUntil(&ultimoDespertar, pdMS_TO_TICKS(periodoAmostraMs));
  }

  vTaskDelete(NULL);      // o trabalho acabou: a task se encerra
}

// Consumidor: mostra na tela e grava no arquivo.
//
// Repare que aqui NÃO usamos mutex no I2C: esta é a única task que fala
// com o display, e o setup() fica bloqueado no semáforo enquanto ela roda.
// Se você somar uma segunda task que também escreva no display ou num
// sensor I2C, aí sim é obrigatório um mutex em volta do barramento.
void tarefaDisplay(void *parametros) {
  File f = LittleFS.open(ARQUIVO_LOG, FILE_APPEND);
  if (!f) {
    Serial.println("ERRO: nao abriu o arquivo");
    xSemaphoreGive(fimDaColeta);
    vTaskDelete(NULL);
  }

  Leitura L;
  int recebidas = 0;

  while (recebidas < AMOSTRAS_POR_CICLO) {
    // Timeout generoso: se o produtor morrer, não ficamos presos para sempre.
    if (xQueueReceive(fila, &L, pdMS_TO_TICKS(2000)) != pdTRUE) {
      Serial.println("timeout esperando a fila");
      break;
    }

    f.printf("%u,%lu,%d\n", boots, L.ms, L.valor);
    recebidas++;
    linhasNoArquivo++;

    Serial.printf("[%2d/%d] %lu ms -> %4d   (na fila: %d)\n",
                  recebidas, AMOSTRAS_POR_CICLO, L.ms, L.valor,
                  uxQueueMessagesWaiting(fila));

    atualizaTela(L.valor, recebidas);
  }

  f.close();                           // sem close(), o dado pode se perder
  xSemaphoreGive(fimDaColeta);         // avisa o setup
  vTaskDelete(NULL);
}

// =============================================================== setup
void setup() {
  Serial.begin(115200);
  delay(300);

  boots++;
  motivo = motivoDoDespertar();
  Serial.println();
  Serial.printf("=== boot #%u  (%s) ===\n", boots, motivo);

  iniciaDisplay();

  // --- Preferences: o intervalo de amostragem sobrevive a tirar da tomada.
  // Leitura sempre; escrita só na primeira vez (a flash tem escritas contadas).
  prefs.begin("logger", false);              // namespace: até 15 caracteres
  if (!prefs.isKey("intervalo")) {
    prefs.putInt("intervalo", PERIODO_PADRAO_MS);
  }
  periodoAmostraMs = prefs.getInt("intervalo", PERIODO_PADRAO_MS);
  Serial.printf("intervalo de amostragem: %d ms\n", periodoAmostraMs);

  // --- LittleFS
  if (!LittleFS.begin(true)) {
    Serial.println("ERRO: LittleFS nao montou. Confira o platformio.ini.");
    mostra("ERRO", "LittleFS");
    delay(5000);
    dorme();
  }
  linhasNoArquivo = contaLinhas();
  Serial.printf("arquivo: %d linhas, %u de %u bytes usados\n", linhasNoArquivo,
                LittleFS.usedBytes(), LittleFS.totalBytes());

  // --- cria fila e semáforo ANTES das tasks que os usam
  fila = xQueueCreate(10, sizeof(Leitura));
  fimDaColeta = xSemaphoreCreateBinary();
  if (fila == NULL || fimDaColeta == NULL) {
    Serial.println("ERRO: fila ou semaforo nao criados");
    while (true) delay(100);
  }

  // --- roda um ciclo de coleta
  xTaskCreatePinnedToCore(tarefaDisplay,  "display",  8192, NULL, 1, NULL, 1);
  xTaskCreatePinnedToCore(tarefaSensor,   "sensor",   4096, NULL, 2, NULL, 1);

  // Espera as tasks terminarem. Timeout de segurança para não travar aqui.
  xSemaphoreTake(fimDaColeta, pdMS_TO_TICKS(30000));

  Serial.printf("ciclo concluido: %d linhas no arquivo\n", linhasNoArquivo);
  prefs.end();

  dorme();
}

void loop() {
  // Nunca chega aqui: o deep sleep reinicia tudo pelo setup().
}

// =============================================================== auxiliares
void dorme() {
  mostra("dormindo", "ate logo");

  esp_sleep_enable_timer_wakeup(SEGUNDOS_DORMINDO * 1000000ULL);
  esp_sleep_enable_ext0_wakeup(PINO_ACORDA, 1);

  Serial.printf("dormindo por %llu s...\n\n", SEGUNDOS_DORMINDO);
  Serial.flush();
  esp_deep_sleep_start();
}

int contaLinhas() {
  File f = LittleFS.open(ARQUIVO_LOG, FILE_READ);
  if (!f) return 0;                    // ainda não existe: primeira execução

  int n = 0;
  while (f.available()) {
    if (f.read() == '\n') n++;
  }
  f.close();
  return n;
}

void iniciaDisplay() {
  Wire.begin(21, 22);
  temDisplay = display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  if (!temDisplay) {
    Serial.println("display nao encontrado (seguindo so pelo serial)");
  }
}

void atualizaTela(int valor, int n) {
  if (!temDisplay) return;

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  display.setTextSize(1);
  display.setCursor(0, 0);
  display.printf("boot #%u  %s", boots, motivo);

  display.setTextSize(2);
  display.setCursor(0, 16);
  display.printf("%4d", valor);

  display.setTextSize(1);
  display.setCursor(0, 40);
  display.printf("amostra %d/%d", n, AMOSTRAS_POR_CICLO);
  display.setCursor(0, 52);
  display.printf("linhas no arquivo: %d", linhasNoArquivo);

  display.display();
}

void mostra(const char *titulo, const char *linha) {
  if (!temDisplay) return;
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(2);
  display.setCursor(0, 8);
  display.println(titulo);
  display.setTextSize(1);
  display.setCursor(0, 40);
  display.println(linha);
  display.display();
}

// Textos curtos: vão para a primeira linha do display, que tem 21 caracteres.
// "reset" cobre a primeira ligação e o botão EN, que não vêm do sono.
const char *motivoDoDespertar() {
  switch (esp_sleep_get_wakeup_cause()) {
    case ESP_SLEEP_WAKEUP_TIMER:    return "timer";
    case ESP_SLEEP_WAKEUP_EXT0:     return "botao";
    case ESP_SLEEP_WAKEUP_TOUCHPAD: return "toque";
    default:                        return "reset";
  }
}
