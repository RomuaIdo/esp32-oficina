/*
 * Oficina de ESP32 — PETECO / UTFPR
 * 03. FreeRTOS, flash e energia — deep sleep com dois jeitos de acordar
 *
 * O programa acorda, faz o trabalho, e volta a dormir. Repare que TUDO
 * acontece no setup(): depois do deep sleep o programa recomeça do zero,
 * como se fosse um reset. O loop() nunca executa.
 *
 * Montagem
 *   Sensor : LDR ou potenciômetro no GPIO 34 (ADC1)
 *   Botão  : GPIO 33 -> botão -> 3V3, com resistor de 10k do GPIO 33 ao GND
 *            (pull-DOWN: o pino fica em LOW e vai a HIGH quando apertado)
 *
 * Por que o GPIO 33: só os RTC GPIO acordam a placa.
 *   0, 2, 4, 12-15, 25-27, 32-39
 *
 * Para medir o consumo: multímetro em série com a alimentação.
 * Acordado fica na casa das dezenas de mA; dormindo, de microampères.
 */

#define uS_POR_SEGUNDO 1000000ULL
const uint64_t SEGUNDOS_DORMINDO = 15;

const int PINO_SENSOR = 34;
const gpio_num_t PINO_ACORDA = GPIO_NUM_33;

// RTC_DATA_ATTR mora na RTC RAM (8 KB), que continua alimentada durante
// o deep sleep. Sobrevive ao sono, mas NÃO sobrevive ao botão de reset
// nem a tirar da tomada — para isso é Preferences ou LittleFS.
RTC_DATA_ATTR uint32_t boots = 0;
RTC_DATA_ATTR int ultimaLeitura = 0;

// Protótipo: a Arduino IDE o gera sozinha, o PlatformIO não.
const char *motivoDoDespertar();

void setup() {
  Serial.begin(115200);
  delay(300);                       // dá tempo de o monitor serial reconectar

  boots++;

  Serial.println();
  Serial.printf("=== boot #%u ===\n", boots);
  Serial.printf("acordei por: %s\n", motivoDoDespertar());
  if (boots > 1) {
    Serial.printf("leitura do ciclo anterior: %d\n", ultimaLeitura);
  }

  // ------------------------------------------------- o trabalho do ciclo
  ultimaLeitura = analogRead(PINO_SENSOR);
  Serial.printf("leitura de agora: %d\n", ultimaLeitura);

  // ------------------------------------------------- configura o despertar
  // 1) por tempo
  esp_sleep_enable_timer_wakeup(SEGUNDOS_DORMINDO * uS_POR_SEGUNDO);

  // 2) por pino: acorda quando o GPIO 33 for a nível ALTO (o 1 no 2º arg)
  esp_sleep_enable_ext0_wakeup(PINO_ACORDA, 1);

  Serial.printf("dormindo por %llu s (ou ate apertar o botao)...\n",
                SEGUNDOS_DORMINDO);

  Serial.flush();                   // esvazia a serial ANTES de dormir,
                                    // senão a última linha se perde
  esp_deep_sleep_start();

  // Nada abaixo desta linha executa.
  Serial.println("esta linha nunca aparece");
}

void loop() {
  // Nunca chega aqui: o deep sleep sempre reinicia pelo setup().
}

// Traduz o motivo do despertar. Na primeira ligação (energia aplicada
// agora, ou botão de reset) o valor é UNDEFINED — é assim que você
// distingue um boot novo de um retorno do sono.
const char *motivoDoDespertar() {
  switch (esp_sleep_get_wakeup_cause()) {
    case ESP_SLEEP_WAKEUP_TIMER:    return "timer";
    case ESP_SLEEP_WAKEUP_EXT0:     return "botao (ext0)";
    case ESP_SLEEP_WAKEUP_EXT1:     return "varios pinos (ext1)";
    case ESP_SLEEP_WAKEUP_TOUCHPAD: return "toque capacitivo";
    case ESP_SLEEP_WAKEUP_ULP:      return "coprocessador ULP";
    default:                        return "ligacao normal ou reset";
  }
}
