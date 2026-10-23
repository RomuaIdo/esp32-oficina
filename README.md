# Oficinas de ESP32 — PETECO

Material da série de oficinas de ESP32 do **PETECO — PET Engenharia de Computação, UTFPR**: slides, código das práticas e guia de montagem dos circuitos.

| | Oficina | Slides | Código |
|---|---|---|---|
| 1 | Fundamentos e o que muda em relação ao Arduino | [`01-fundamentos-esp32.md`](slides/01-fundamentos-esp32.md) | `exemplos/01*` |
| 2 | PWM, timers e protocolos (I2C, SPI) | [`02-pwm-e-protocolos.md`](slides/02-pwm-e-protocolos.md) | `exemplos/02*` |
| 3 | FreeRTOS, memória flash e energia | [`03-freertos-flash-energia.md`](slides/03-freertos-flash-energia.md) | `exemplos/03*` |
| 4 | Wi-Fi e IoT: servidor web, MQTT, NTP e OTA | [`04-wifi-e-iot.md`](slides/04-wifi-e-iot.md) | `exemplos/04*` |

Os PDFs dos slides ficam na página de **Releases** do repositório. Para gerar a partir do código-fonte, veja [`slides/README.md`](slides/README.md).

## Para quem está fazendo a oficina

- **[`exemplos/`](exemplos/README.md)** — o código de cada prática, em duas versões com o mesmo conteúdo: Arduino IDE (`.ino`) e PlatformIO (`exemplos/platformio/`)
- **[`exemplos/MONTAGEM.md`](exemplos/MONTAGEM.md)** — como montar cada circuito na protoboard, fio a fio, e como saber se deu certo
- **[`exemplos/so-placa/`](exemplos/so-placa/README.md)** — oito demonstrações que só precisam da DevKit, sem nenhum componente externo

**O que você precisa:** uma ESP32 DevKit (WROOM-32), cabo USB de dados e a Arduino IDE 2 com o pacote *esp32 by Espressif Systems* na versão **3.x**. Os componentes de cada prática estão no `MONTAGEM.md`.

> Os exemplos usam o core **3.x**. Muito tutorial na internet ainda é do 2.x, e funções como `ledcSetup` e `SYSTEM_EVENT_*` não compilam mais.

## Estrutura

```
.
├── exemplos/        ← código das práticas e guia de montagem
│   ├── 01a-…/ … 04d-…/      Arduino IDE
│   ├── platformio/          as mesmas práticas, para o PlatformIO
│   ├── so-placa/            demonstrações sem componentes
│   └── MONTAGEM.md
└── slides/          ← fonte dos slides (Marp), tema, imagens e ferramentas
```

## Para quem vai dar ou editar a oficina

Os slides são Markdown com o tema do PETECO. Tudo o que é preciso para compilar, criar uma oficina nova e conferir se algum slide estourou a altura está em [`slides/README.md`](slides/README.md). Antes da Oficina 4, leia [`slides/preparacao-oficina-04.md`](slides/preparacao-oficina-04.md): ela depende de roteador e broker MQTT montados antes da aula.
