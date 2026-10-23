/*
 * Oficina de ESP32 — PETECO / UTFPR
 * 03. FreeRTOS, flash e energia — Preferences (NVS) e LittleFS
 *
 * Mostra as duas formas de guardar dado na flash:
 *   Preferences -> valores pequenos, chave-valor (configuração, contadores)
 *   LittleFS    -> arquivos (log, histórico, JSON)
 *
 * ANTES DE GRAVAR: Ferramentas > Partition Scheme > um esquema com SPIFFS
 * (por exemplo "Default 4MB with spiffs"). Sem isso o LittleFS não monta.
 *
 * Teste que vale fazer: rode, tire da tomada, ligue de novo.
 * O contador de boots e o arquivo continuam lá.
 */

#include <Preferences.h>
#include <LittleFS.h>

const int PINO_SENSOR = 34;
const char *ARQUIVO_LOG = "/log.csv";

Preferences prefs;

uint32_t boots = 0;
int intervaloMs = 1000;
int intervaloSalvo = 0;      // cópia do que já está na flash

// Protótipos: a Arduino IDE os gera sozinha, o PlatformIO não.
void gravaLeitura();
void atendeComandos();
void despejaArquivo();
int  contaLinhas();

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("\n=== Persistencia ===");

  // ------------------------------------------------- Preferences (NVS)
  // Namespace e chave: NO MÁXIMO 15 CARACTERES cada.
  // "intervaloAmostragem" tem 19 e falharia silenciosamente.
  prefs.begin("oficina3", false);          // false = leitura e escrita

  boots = prefs.getUInt("boots", 0) + 1;   // 2º arg: padrão na 1ª execução
  intervaloMs = prefs.getInt("intervalo", 1000);
  intervaloSalvo = intervaloMs;

  prefs.putUInt("boots", boots);           // este sim muda todo boot

  Serial.printf("boot #%u\n", boots);
  Serial.printf("intervalo salvo: %d ms\n", intervaloMs);

  // ------------------------------------------------- LittleFS
  if (!LittleFS.begin(true)) {             // true = formata se falhar
    Serial.println("ERRO: LittleFS nao montou.");
    Serial.println("Confira o Partition Scheme na IDE.");
    while (true) delay(100);
  }

  Serial.printf("LittleFS: %u de %u bytes usados\n",
                LittleFS.usedBytes(), LittleFS.totalBytes());
  Serial.printf("linhas ja gravadas: %d\n", contaLinhas());

  // Marca o começo desta sessão no arquivo.
  File f = LittleFS.open(ARQUIVO_LOG, FILE_APPEND);
  if (f) {
    f.printf("# boot %u\n", boots);
    f.close();
  }

  Serial.println("comandos: d = despejar arquivo | a = apagar tudo");
}

// ----------------------------------------------------------- loop
void loop() {
  gravaLeitura();
  atendeComandos();
}

// Grava uma linha no CSV, no ritmo guardado no Preferences.
void gravaLeitura() {
  static unsigned long ultima = 0;
  if (millis() - ultima < (unsigned long)intervaloMs) return;
  ultima = millis();

  int valor = analogRead(PINO_SENSOR);

  File f = LittleFS.open(ARQUIVO_LOG, FILE_APPEND);
  if (!f) {
    Serial.println("falha ao abrir o arquivo");
    return;
  }
  f.printf("%lu,%d\n", millis(), valor);
  f.close();                               // sem close(), o dado pode se perder

  Serial.printf("gravado: %lu,%d\n", millis(), valor);
}

void atendeComandos() {
  if (!Serial.available()) return;

  char c = Serial.read();
  if (c == 'd') {
    despejaArquivo();
  } else if (c == 'a') {
    LittleFS.remove(ARQUIVO_LOG);
    prefs.clear();                         // limpa o namespace inteiro
    Serial.println("arquivo e preferencias apagados");
  }
}

void despejaArquivo() {
  File f = LittleFS.open(ARQUIVO_LOG, FILE_READ);
  if (!f) { Serial.println("arquivo nao existe"); return; }

  Serial.println("----- inicio do arquivo -----");
  while (f.available()) Serial.write(f.read());
  Serial.println("----- fim do arquivo -----");
  f.close();
}

int contaLinhas() {
  File f = LittleFS.open(ARQUIVO_LOG, FILE_READ);
  if (!f) return 0;

  int n = 0;
  while (f.available()) {
    if (f.read() == '\n') n++;
  }
  f.close();
  return n;
}

/*
 * Como mudar o intervalo sem reprogramar a placa:
 *
 *   if (novoIntervalo != intervaloSalvo) {   // SÓ grave quando MUDA
 *     prefs.putInt("intervalo", novoIntervalo);
 *     intervaloSalvo = novoIntervalo;
 *   }
 *
 * Gravar a cada volta do loop desgasta a flash e destrói a partição
 * em poucos dias. O teste "mudou?" é o que evita isso.
 */
