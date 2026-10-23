# Oficina 4 — preparação do instrutor

Esta é a oficina que mais depende de infraestrutura. Leia antes de dar a aula.

## Antes da oficina

**Leve um roteador e ligue antes de a turma chegar.** O eduroam usa WPA2-Enterprise, que o ESP32 trata mal (detalhes no slide). Sem rede própria, a oficina inteira trava no primeiro slide de código. Deixe o SSID e a senha escritos no quadro.

**Suba o Mosquitto no seu notebook e anote o IP da máquina na rede do roteador.** Os alunos vão precisar dele:

```bash
sudo apt install mosquitto mosquitto-clients
printf 'listener 1883\nallow_anonymous true\n' > oficina.conf
mosquitto -c oficina.conf -v
```

Sem esse arquivo, o Mosquitto 2.x só escuta em `localhost`: funciona nos seus testes com `mosquitto_sub` e nenhuma placa conecta. Se o pacote da distribuição já subiu um serviço na 1883, pare antes com `sudo systemctl stop mosquitto`.

Se o firewall do notebook bloquear a porta 1883, ninguém conecta — e o sintoma (estado `-2`) não aponta para isso. Teste de outra máquina antes.

**Deixe o MQTT Explorer aberto e projetado.** Ver as mensagens das bancadas chegando ao vivo é o que faz o conceito de publish/subscribe assentar. Vale mais que qualquer slide.

**Distribua os nomes de bancada** (`b1`, `b2`, …) antes de começar. Duas bancadas publicando no mesmo tópico, ou com o mesmo client ID, se derrubam mutuamente — e o diagnóstico é confuso.

**Se o tempo apertar,** o corte mais seguro é o bloco de OTA: é o único que não entra na prática.

---

## Detalhes técnicos que os slides cobrem

Pontos verificados na documentação da Espressif e nos exemplos oficiais do core:

- **Os nomes dos eventos de Wi-Fi mudaram no core 3.x**: hoje são `ARDUINO_EVENT_WIFI_*`. Material com `SYSTEM_EVENT_*` é da versão 2.x e não compila.
- **Os callbacks de evento do Wi-Fi rodam em outra task do FreeRTOS** — o próprio exemplo da Espressif avisa isso em comentário. Os slides ligam esse ponto de volta ao conteúdo da Oficina 3 (concorrência, filas, mutex).
- **WPA2-Enterprise (eduroam) é genuinamente problemático** no ESP32: há histórico de reinício em laço conforme a versão do core e a configuração de PSRAM. Os slides apresentam isso com honestidade e dão as três alternativas práticas.
- **O `PubSubClient` tem buffer padrão de 256 bytes** e **as assinaturas não sobrevivem à reconexão** — duas causas de bug que parecem aleatórias.

O exemplo `04d` demonstra na prática uma consequência da Oficina 3: como o `PubSubClient` não é thread-safe, o servidor web **não** publica direto. Ele levanta uma flag protegida por mutex e a task de MQTT publica por ele — o mesmo princípio da ISR que só levanta uma flag.
