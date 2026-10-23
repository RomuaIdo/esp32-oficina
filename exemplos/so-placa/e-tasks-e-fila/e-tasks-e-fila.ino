/*
 * Oficina de ESP32 — PETECO / UTFPR
 * Só a placa — duas tasks, duas filas e uma ISR             (tema da oficina 3)
 *
 * Nada para montar: botão BOOT (GPIO 0) e LED azul (GPIO 2) da DevKit.
 *
 *   ISR do BOOT --filaApertos--> tarefaBotao --filaPeriodo--> tarefaPisca
 *
 * - A ISR só manda o instante do aperto para uma fila (versão FromISR).
 * - tarefaBotao faz o debounce, escolhe o próximo ritmo e avisa a outra task.
 * - tarefaPisca pisca o LED. Ela espera na fila com timeout = meio período:
 *   se chegar um ritmo novo, troca; se der timeout, é hora de piscar.
 *
 * Nenhuma variável global é compartilhada entre as tasks: só filas.
 */

const int PINO_BOTAO = 0;
const int PINO_LED = 2;

const uint32_t PERIODOS_MS[] = { 1000, 400, 100 };
const int N_PERIODOS = sizeof(PERIODOS_MS) / sizeof(PERIODOS_MS[0]);

QueueHandle_t filaApertos;               // ISR -> tarefaBotao (instante do aperto)
QueueHandle_t filaPeriodo;               // tarefaBotao -> tarefaPisca (novo ritmo)

// Protótipos: a Arduino IDE os gera sozinha, o PlatformIO não.
void tarefaBotao(void *parametros);
void tarefaPisca(void *parametros);

void IRAM_ATTR aoApertar() {
  BaseType_t acordouMaisImportante = pdFALSE;
  uint32_t agora = millis();
  xQueueSendFromISR(filaApertos, &agora, &acordouMaisImportante);
  if (acordouMaisImportante) portYIELD_FROM_ISR();
}

void setup() {
  Serial.begin(115200);
  delay(500);
  pinMode(PINO_LED, OUTPUT);
  pinMode(PINO_BOTAO, INPUT_PULLUP);

  // As filas existem ANTES de qualquer um que as use: tasks e interrupção.
  filaApertos = xQueueCreate(10, sizeof(uint32_t));
  filaPeriodo = xQueueCreate(1, sizeof(uint32_t));

  xTaskCreatePinnedToCore(tarefaPisca, "pisca", 4096, NULL, 1, NULL, 1);
  xTaskCreatePinnedToCore(tarefaBotao, "botao", 4096, NULL, 2, NULL, 1);

  attachInterrupt(digitalPinToInterrupt(PINO_BOTAO), aoApertar, FALLING);
  Serial.println("aperte BOOT para trocar o ritmo do LED");
}

void loop() {
  vTaskDelay(pdMS_TO_TICKS(1000));       // o loop() não tem nada a fazer
}

void tarefaBotao(void *parametros) {
  Serial.printf("tarefaBotao no nucleo %d\n", xPortGetCoreID());
  uint32_t instante;
  uint32_t ultimoAceito = 0;
  int indice = 0;

  for (;;) {
    // Bloqueada sem gastar CPU até a ISR mandar alguma coisa.
    xQueueReceive(filaApertos, &instante, portMAX_DELAY);

    if (instante - ultimoAceito < 200) continue;   // repique do contato
    ultimoAceito = instante;

    indice = (indice + 1) % N_PERIODOS;
    uint32_t periodo = PERIODOS_MS[indice];
    xQueueOverwrite(filaPeriodo, &periodo);        // fila de 1: fica o mais novo
    Serial.printf("aperto em %lu ms -> LED pisca a cada %lu ms\n",
                  (unsigned long)instante, (unsigned long)periodo);
  }
}

void tarefaPisca(void *parametros) {
  Serial.printf("tarefaPisca no nucleo %d\n", xPortGetCoreID());
  uint32_t periodo = PERIODOS_MS[0];
  bool aceso = false;

  for (;;) {
    uint32_t novo;
    if (xQueueReceive(filaPeriodo, &novo, pdMS_TO_TICKS(periodo / 2)) == pdTRUE) {
      periodo = novo;                    // chegou ritmo novo
    } else {
      aceso = !aceso;                    // timeout: hora de piscar
      digitalWrite(PINO_LED, aceso);
    }
  }
}
