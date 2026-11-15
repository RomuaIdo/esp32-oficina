# Oficina 5 — preparação do instrutor

Última da série. Esta aula depende de preparo: os MACs das placas e o gateway precisam estar prontos antes de a turma chegar.

## Antes da oficina

**Grave o `05a` em todas as 12 placas e monte a tabela de MACs no quadro.** Sem isso, a prática de ESP-NOW não começa — é o preparo mais importante desta aula.

**Suba o `05d` na sua placa e deixe o painel projetado desde o início.** Ver as bancadas aparecerem uma a uma conforme a turma liga as placas é o que dá ritmo à prática final.

**O `05d` vem com `MODO_AP = true`.** Assim o gateway cria a própria rede (`PETECO-Gateway`, senha `oficina5esp32`) e **você escolhe o canal**, o que elimina de uma vez o problema de canal descrito abaixo. Conecte o notebook do projetor nessa rede e abra `http://192.168.4.1`. Se preferir ligar o gateway ao roteador, mude para `false` e avise a turma do canal que aparecer no serial.

**O MAC do gateway que vai no quadro é o que o próprio `05d` imprime** (`MAC do gateway (para MAC_GATEWAY no 05b)`), igual ao do `05a`. Ele é o MAC de estação; o ponto de acesso tem outro, e mandar para ele não funciona.

**Peça que instalem o nRF Connect no celular antes de vir.** Baixar o app na hora custa dez minutos da aula.

**Se o tempo apertar,** transforme o bloco de BLE numa demonstração sua em vez de prática na bancada. O ESP-NOW é o que entra na prática final.

---

## Os três pontos que derrubam a prática

**1. O canal.** ESP-NOW e Wi-Fi dividem o mesmo rádio, que escuta um canal por vez. Uma placa desconectada fica no canal 1; uma conectada fica no canal do roteador. Se não baterem, **as mensagens somem sem nenhum erro** — tudo parece certo no código. É a causa da maioria dos "não recebo nada", e tem um slide de alerta só para isso.

**2. As assinaturas dos callbacks mudaram no core 3.x.** Hoje são:

```cpp
// recepção, desde o core 3.0
void aoReceber(const esp_now_recv_info_t *info, const uint8_t *dados, int len)
// remetente em info->src_addr

// envio, desde o core 3.3 (ESP-IDF 5.5)
void aoEnviar(const esp_now_send_info_t *info, esp_now_send_status_t status)
// destino em info->des_addr
```

Quase todo tutorial e vídeo de ESP-NOW usa as formas antigas `(const uint8_t *mac, ...)`, que dão erro de *invalid conversion*. O `05b` usa um `#if ESP_IDF_VERSION` para compilar nas duas versões do callback de envio.

**3. A NimBLE 2.x quebrou vários exemplos da 1.x.** Os pontos que aparecem num servidor simples, todos conferidos na documentação da biblioteca:

| 1.x | 2.x |
|---|---|
| `onWrite(NimBLECharacteristic*)` | `onWrite(NimBLECharacteristic*, NimBLEConnInfo&)` |
| `onConnect(NimBLEServer*)` | `onConnect(NimBLEServer*, NimBLEConnInfo&)` |
| — | `onDisconnect(NimBLEServer*, NimBLEConnInfo&, int reason)` |
| `setScanResponse(true)` | `enableScanResponse(true)` |
| anunciava o nome sozinho | é preciso chamar `setName()` |
| voltava a anunciar sozinho | é preciso `advertiseOnDisconnect(true)` |
| `setPower(ESP_PWR_LVL_P9)` | `setPower(9)`, em dBm |

---

## Fechando a série

Os dois últimos slides de conteúdo são o fechamento das cinco oficinas: um resumo do que ficou de cada uma e um **checklist para a entrega do projeto** (mapa de pinos, tensões, alimentação de atuadores, persistência, credenciais fora do repositório, plano B de rede, e testar com a caixa fechada).

Vale reservar 20 minutos no fim para cada equipe falar do próprio projeto e discutir qual arquitetura faz sentido. É o que amarra a série toda, e rende mais que qualquer slide extra.
