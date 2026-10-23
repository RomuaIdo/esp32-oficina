#include <Arduino.h>

/*
 * Oficina de ESP32 — PETECO / UTFPR
 * 03. FreeRTOS, flash e energia — duas tasks conversando por fila
 *
 * Mostra o padrão produtor/consumidor, que é a arquitetura padrão de
 * projeto embarcado: uma task lê rápido, outra processa devagar, e a
 * fila absorve a diferença sem perder dado.
 *
 * Experimento proposto
 *   1. Rode como está: o consumidor (50 ms) é mais rápido que o produtor
 *      (100 ms), então a fila fica vazia: "na fila= 0".
 *   2. Troque CONSUMIDOR_LENTO_MS para 2000. Agora o consumidor é mais lento:
 *      as leituras se ACUMULAM na fila em vez de se perderem, e "na fila"
 *      sobe até 10 em cerca de 1 segundo.
 *   3. Com a fila cheia, o produtor passa a descartar (xQueueSend usa
 *      timeout 0) e o contador de descartadas sobe.
 *   4. Troque o timeout do xQueueSend para portMAX_DELAY: nada mais se
 *      perde, mas o produtor passa a andar no ritmo do consumidor.
 *
 * Montagem
 *   LDR ou potenciômetro no GPIO 34 (ADC1)
 */

const int PINO_SENSOR = 34;

const int TAMANHO_FILA        = 10;
const int PRODUTOR_MS         = 100;    // 10 leituras por segundo
const int CONSUMIDOR_LENTO_MS = 50;     // troque para 2000 e observe

// O dado que trafega na fila. Mantenha a struct pequena:
// a fila COPIA cada item, então o custo é tamanho x capacidade.
struct Leitura {
  uint32_t ms;
  int      valor;
};

QueueHandle_t fila;
volatile uint32_t descartadas = 0;

// ------------------------------------------------------- produtor
void tarefaSensor(void *parametros) {
  Serial.printf("tarefaSensor no nucleo %d\n", xPortGetCoreID());

  // vTaskDelayUntil mantém o período EXATO, sem somar o tempo gasto
  // no próprio trabalho. É o certo para amostragem periódica.
  TickType_t ultimoDespertar = xTaskGetTickCount();

  for (;;) {
    Leitura L = { millis(), analogRead(PINO_SENSOR) };

    // Timeout 0: se a fila estiver cheia, não espera — descarta e segue.
    // É a escolha certa quando perder uma amostra é melhor que atrasar.
    if (xQueueSend(fila, &L, 0) != pdTRUE) {
      descartadas = descartadas + 1;   // "++" em volatile gera aviso no C++20
    }

    vTaskDelayUntil(&ultimoDespertar, pdMS_TO_TICKS(PRODUTOR_MS));
  }
}

// ------------------------------------------------------- consumidor
void tarefaProcessa(void *parametros) {
  Serial.printf("tarefaProcessa no nucleo %d\n", xPortGetCoreID());

  Leitura L;
  for (;;) {
    // portMAX_DELAY: fica BLOQUEADA sem gastar CPU até chegar um item.
    if (xQueueReceive(fila, &L, portMAX_DELAY) == pdTRUE) {

      Serial.printf("t=%6lu ms  valor=%4d  na fila=%2d  descartadas=%lu\n",
                    L.ms, L.valor,
                    uxQueueMessagesWaiting(fila),
                    descartadas);

      // Simula um trabalho lento: gravar em cartão, desenhar no display,
      // enviar pela rede. Enquanto isso, o produtor continua lendo.
      vTaskDelay(pdMS_TO_TICKS(CONSUMIDOR_LENTO_MS));
    }
  }
}

// ------------------------------------------------------- setup
void setup() {
  Serial.begin(115200);
  delay(500);

  Serial.printf("loop() roda no nucleo %d\n", xPortGetCoreID());

  // A fila precisa existir ANTES das tasks que a usam.
  fila = xQueueCreate(TAMANHO_FILA, sizeof(Leitura));
  if (fila == NULL) {
    Serial.println("ERRO: nao foi possivel criar a fila");
    while (true) delay(100);
  }

  // Lembre: a pilha é em BYTES no ESP32, não em words.
  xTaskCreatePinnedToCore(tarefaSensor,   "sensor",   4096, NULL, 2, NULL, 1);
  xTaskCreatePinnedToCore(tarefaProcessa, "processa", 4096, NULL, 1, NULL, 1);
}

void loop() {
  // O loop() é só mais uma task. Com o trabalho nas outras duas,
  // ele não tem nada a fazer — mas precisa ceder a CPU mesmo assim.
  vTaskDelay(pdMS_TO_TICKS(1000));
}
