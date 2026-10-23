# Exemplos das oficinas

Cada exemplo existe em duas versões, com o mesmo código:

```
exemplos/
├── 01a-primeiro-sketch/          ← Arduino IDE (.ino)
├── 01b-ldr-led/
├── 01c-botao-interrupcao/
├── 02a-scanner-i2c/
├── 02b-pwm-led-buzzer/
├── 02c-oled-sensor-pwm/
├── 02d-timer-hardware/
├── 03a-tasks-e-fila/
├── 03b-persistencia/
├── 03c-deep-sleep/
├── 03d-datalogger-completo/
├── 04a-wifi-conexao/
├── 04b-servidor-web/
├── 04c-mqtt/
├── 04d-painel-completo/
├── so-placa/                     ← demonstrações sem nenhum componente externo
└── platformio/                   ← PlatformIO (VS Code)
    ├── 01a-primeiro-sketch/
    │   ├── platformio.ini
    │   └── src/main.cpp
    ├── 01b-ldr-led/
    ├── ...
    └── 04d-painel-completo/
```

Use a que preferir. Os slides mostram a Arduino IDE, mas tudo funciona igual no PlatformIO.

**Como montar cada circuito:** [`MONTAGEM.md`](MONTAGEM.md).

**Sem componentes, só a placa:** [`so-placa/`](so-placa/README.md) — oito demonstrações com o LED, o botão BOOT, o toque capacitivo e o Wi-Fi da própria DevKit, cada uma com versão Arduino IDE e PlatformIO.

---

## Arduino IDE

Abra o `.ino` (*Arquivo → Abrir*). A pasta precisa ter o mesmo nome do arquivo, por isso cada exemplo fica numa pasta própria.

## PlatformIO

1. Instale o [VS Code](https://code.visualstudio.com/) e a extensão **PlatformIO IDE**
2. *File → Open Folder* e abra **a pasta do exemplo** (ex.: `exemplos/platformio/01b-ldr-led`), não a pasta `exemplos/` inteira. O PlatformIO só reconhece o projeto se o `platformio.ini` estiver na raiz do que foi aberto
3. Na barra inferior: ✓ compila, → grava, 🔌 abre o monitor serial (já configurado em 115200)

Pelo terminal, de dentro da pasta do exemplo:

```bash
pio run                  # compila
pio run -t upload        # grava
pio device monitor       # monitor serial
```

> **A primeira compilação demora.** O PlatformIO baixa a plataforma e o compilador (algumas centenas de MB). Faça isso **antes** da oficina, com internet boa.

### Diferenças em relação à Arduino IDE

| | Arduino IDE | PlatformIO |
|---|---|---|
| Arquivo do código | `nome/nome.ino` | `src/main.cpp` |
| `#include <Arduino.h>` | automático | obrigatório no topo |
| Funções usadas antes de definidas | a IDE declara sozinha | declare o protótipo antes, como em C++ comum |
| Placa e velocidade do monitor | menu *Ferramentas* | `platformio.ini` |
| Partição para o LittleFS (03b, 03d) | *Ferramentas → Partition Scheme* | `board_build.partitions` no `platformio.ini`, já configurado |
| Bibliotecas (Adafruit no 02c e 03d, PubSubClient no 04c e 04d) | Library Manager | `lib_deps` no `platformio.ini`, baixadas sozinhas |
| Serial Plotter | embutido | extensão **Teleplot** do VS Code |

### Por que `pioarduino` no `platformio.ini`

A plataforma oficial `espressif32` do PlatformIO ainda vem com o core Arduino-ESP32 **2.x**. As oficinas usam o **3.x**, em que algumas funções mudaram de nome (PWM e timers, na oficina 2). O [pioarduino](https://github.com/pioarduino/platform-espressif32) é o fork mantido pela comunidade que traz o 3.x — é ele que a linha `platform = ...` aponta.

### Serial Plotter no PlatformIO (exemplo 01b)

O PlatformIO não tem plotter próprio. Instale a extensão **Teleplot**, clique no ícone dela, escolha a porta e 115200. Por isso a versão PlatformIO do `01b` imprime no formato que o Teleplot entende:

```
>bruto:2310
>mV:1874
```

Lembre de **fechar o monitor serial do PlatformIO** antes de conectar o Teleplot (e vice-versa): só um programa usa a porta por vez. O mesmo vale na hora de gravar.

### Se mudar de placa

Troque `board = esp32dev` no `platformio.ini`. Para ver as opções: `pio boards espressif32`. Exemplos: `esp32-s3-devkitc-1`, `esp32-c3-devkitm-1`.
