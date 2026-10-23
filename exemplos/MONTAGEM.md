# Montagem dos circuitos

Guia de bancada para todos os circuitos das oficinas. Cada seção diz o que pegar, onde ligar cada fio e como saber se deu certo.

| Circuito | Oficina | Código |
|---|---|---|
| [0. Só a placa](#0-só-a-placa--verificação-do-ambiente) | 1 | `01a-primeiro-sketch` |
| [1. LDR + LED](#1-ldr--led--leitura-analógica) | 1 | `01b-ldr-led` |
| [2. Botão + LED](#2-botão--led--interrupção) | 1 | `01c-botao-interrupcao` |
| [3. Scanner I2C](#3-módulo-i2c--scanner) | 2 | `02a-scanner-i2c` |
| [4. LED + buzzer + potenciômetro](#4-led--buzzer--potenciômetro--pwm) | 2 | `02b-pwm-led-buzzer` |
| [5. OLED + BMP280 + LED](#5-oled--bmp280--led--prática-da-oficina-2) | 2 | `02c-oled-sensor-pwm` |
| [5b. OLED + MPU6050 + servo](#5b-variante-oled--mpu6050--servo) | 2 | *(sem exemplo pronto)* |
| [6. Timer de hardware](#6-timer-de-hardware) | 2 | `02d-timer-hardware` |
| [7. Sensor analógico — tasks e persistência](#7-sensor-analógico--tasks-fila-e-persistência) | 3 | `03a-tasks-e-fila`, `03b-persistencia` |
| [8. Sensor + botão de despertar](#8-sensor--botão-de-despertar--deep-sleep) | 3 | `03c-deep-sleep` |
| [9. Data logger completo](#9-data-logger-completo--prática-da-oficina-3) | 3 | `03d-datalogger-completo` |
| [Medindo o consumo](#medindo-o-consumo) | 3 | `03c-deep-sleep` |
| [10. Sensor + LED na rede](#10-sensor--led-na-rede--prática-da-oficina-4) | 4 | `04a-wifi-conexao` a `04d-painel-completo` |

---

## Antes de qualquer circuito

**Monte com a placa desligada do USB.** Ligue o cabo só depois de conferir os fios.

**A protoboard.** As duas trilhas compridas nas bordas são os barramentos de alimentação. Ligue o `3V3` da ESP32 numa trilha e o `GND` em outra, e puxe tudo de lá. Em muitas protoboards de 830 pontos a trilha é **cortada no meio**: se um lado não tiver tensão, ligue as duas metades com um jumper.

**A placa.** A DevKit de 38 pinos cobre quase toda a protoboard e deixa só uma fileira livre de cada lado. Se faltar espaço, use duas protoboards lado a lado ou jumpers macho-fêmea direto nos pinos da placa.

**As quatro regras que evitam queimar coisa:**

1. **Nada de 5 V num GPIO.** Os pinos são de 3,3 V. O `VIN` é 5 V e serve só para alimentar módulos e o servo, nunca vai num GPIO
2. **LED sempre com resistor** de 220 Ω em série. Sem ele, o LED puxa corrente demais do pino
3. **GPIO 34 a 39 são só de entrada.** Não acendem LED, e `INPUT_PULLUP` não funciona neles
4. **Não use os GPIO 6 a 11** (memória flash). Deixe os GPIO 0, 2, 5, 12 e 15 por último: eles atrapalham o boot

**Polaridade do LED.** A perna **longa** é o anodo (+) e vai do lado do resistor, ou seja, do GPIO. A perna **curta**, do lado chanfrado do LED, é o catodo (−) e vai no GND. LED invertido não queima, só não acende.

```
GPIO ──[ 220 Ω ]──►|── GND
                 longa  curta
```

---

## 0. Só a placa — verificação do ambiente

**Material:** ESP32 e cabo USB **de dados**.

Não há circuito. Conecte a placa, grave o `01a-primeiro-sketch` e abra o monitor serial a 115200.

**Deu certo se** aparecer `ESP32 vivo!` seguido do modelo do chip, número de núcleos e RAM livre.

**Não apareceu porta?** O cabo pode ser só de carga, ou falta o driver CH340/CP2102. Veja a lista de checagem nos slides da Oficina 1.

---

## 1. LDR + LED — leitura analógica

![Esquema do LDR](../slides/assets/circuito-ldr.svg)

**Material:** 1 LDR, 1 resistor de 10 kΩ, 1 LED, 1 resistor de 220 Ω, jumpers.

O LDR e o resistor de 10 kΩ formam um **divisor de tensão**. O ponto entre eles é o que o GPIO 34 lê.

| De | Para |
|---|---|
| `3V3` | uma perna do LDR (o LDR não tem polaridade) |
| outra perna do LDR | uma fileira livre da protoboard — este é o **nó de leitura** |
| nó de leitura | `GPIO 34` |
| nó de leitura | uma perna do resistor de 10 kΩ |
| outra perna do 10 kΩ | `GND` |
| `GPIO 13` | resistor de 220 Ω |
| resistor de 220 Ω | perna longa do LED |
| perna curta do LED | `GND` |

```
3V3 ──[ LDR ]──┬──[ 10 kΩ ]── GND
               │
            GPIO 34

GPIO 13 ──[ 220 Ω ]──►|── GND
```

**Deu certo se** o valor no monitor (ou no Serial Plotter / Teleplot) **cair** quando você cobre o LDR com a mão. Anote o valor no claro e no escuro e ajuste `LIMIAR_ESCURO` para um valor entre os dois. O LED deve acender quando escurece.

**Problemas comuns**
- Leitura parada em 0: o nó de leitura não está ligado ao 3V3 pelo LDR, ou o fio do GPIO 34 está na fileira errada
- Leitura parada em 4095: o resistor de 10 kΩ não chega ao GND
- Leitura subindo quando escurece: LDR e resistor estão trocados de posição. Funciona, mas a lógica do código fica invertida

---

## 2. Botão + LED — interrupção

![Esquema do botão](../slides/assets/circuito-botao.svg)

**Material:** 1 push button, 1 LED, 1 resistor de 220 Ω, jumpers.

Desmonte o circuito anterior e comece do zero.

O código usa `INPUT_PULLUP`, então o **resistor de 10 kΩ do desenho não é necessário**: o pull-up interno do ESP32 faz o papel dele.

| De | Para |
|---|---|
| `GPIO 4` | uma perna do botão |
| perna **diagonal oposta** do botão | `GND` |
| `GPIO 13` | resistor de 220 Ω → perna longa do LED |
| perna curta do LED | `GND` |

**O botão de 4 pernas.** As pernas são ligadas internamente aos pares, e só as pernas de pares diferentes se conectam quando você aperta. Coloque o botão **atravessando o canal central** da protoboard e use pernas em **diagonal**: assim você acerta sempre, seja qual for a orientação.

```
GPIO 4 ──┐
         ○ ○        botão atravessando o canal central
       ──────       da protoboard
         ○ ○
           └── GND
```

**Deu certo se** cada aperto alternar o LED e imprimir `LED aceso` / `LED apagado`.

**Problemas comuns**
- LED troca sozinho ou várias vezes por aperto: repique do contato. Confira se o `delay(200)` do debounce está no código
- LED nunca troca: pernas do mesmo par (sempre ligadas, ou nunca). Gire o botão 90° ou use a diagonal
- Se mudar o botão para um GPIO de 34 a 39, o pull-up interno não existe: aí o resistor de 10 kΩ entre o pino e o `3V3` volta a ser obrigatório

---

## 3. Módulo I2C — scanner

![Ligação do OLED no I2C](../slides/assets/circuito-oled-i2c.svg)

**Material:** o display OLED SSD1306 e/ou o sensor (BMP280 ou MPU6050), jumpers.

Todo módulo I2C usa os **mesmos quatro fios**:

| Pino do módulo | Liga em |
|---|---|
| `VCC` (ou `VIN`) | `3V3` |
| `GND` | `GND` |
| `SDA` | `GPIO 21` |
| `SCL` | `GPIO 22` |

> **Leia a serigrafia do módulo.** A ordem dos pinos muda de fabricante para fabricante: há OLED com `GND VCC SCL SDA` e outros com `VCC GND SCL SDA`. Trocar VCC e GND pode queimar o módulo.

Para ligar **vários módulos**, coloque todos no **mesmo barramento**: todos os SDA juntos no GPIO 21, todos os SCL juntos no GPIO 22. Dá para fazer isso com duas fileiras da protoboard, uma para SDA e uma para SCL.

```
           ┌── OLED SDA
GPIO 21 ───┼── sensor SDA      (fileira "SDA")
           │
           ┌── OLED SCL
GPIO 22 ───┼── sensor SCL      (fileira "SCL")
```

Os módulos de breakout já trazem os resistores de pull-up soldados, então não é preciso acrescentar nada.

**Deu certo se** o scanner listar o endereço de cada módulo:

| Módulo | Endereço esperado |
|---|---|
| OLED SSD1306 | `0x3C` (alguns `0x3D`) |
| BMP280 | `0x76` (alguns `0x77`) |
| MPU6050 | `0x68` |

**Como ler o resultado do scanner**
- Nenhum dispositivo: fio solto, sem alimentação ou SDA/SCL invertidos. Comece trocando os dois de lugar
- Todos os 127 endereços: SDA em curto com o GND
- Endereço diferente da tabela: é variante do módulo. Use o endereço que apareceu e troque a constante no código

---

## 4. LED + buzzer + potenciômetro — PWM

**Material:** 1 LED, 1 resistor de 220 Ω, 1 **buzzer passivo**, 1 potenciômetro de 10 kΩ, jumpers.

| De | Para |
|---|---|
| `GPIO 13` | resistor de 220 Ω → perna longa do LED |
| perna curta do LED | `GND` |
| `GPIO 25` | perna **+** do buzzer |
| perna − do buzzer | `GND` |
| perna **externa** do potenciômetro | `3V3` |
| outra perna **externa** do potenciômetro | `GND` |
| perna **do meio** do potenciômetro (cursor) | `GPIO 34` |

```
GPIO 13 ──[ 220 Ω ]──►|── GND

GPIO 25 ──(+ buzzer −)── GND

3V3 ──┤ pot ├── GND
        │
     GPIO 34
```

**Buzzer passivo ou ativo?** O código precisa do **passivo**: é ele que toca a frequência que o PWM manda. Para diferenciar: o ativo costuma ter um adesivo por cima e o fundo fechado com resina; o passivo deixa a placa de circuito visível por baixo. Ligado direto em 3V3, o ativo apita sozinho e o passivo só dá um estalo.

**Deu certo se** o LED fizer um fade ao ligar, o buzzer der um bipe, e depois o brilho do LED acompanhar o potenciômetro.

**Problemas comuns**
- Brilho anda ao contrário: as pernas externas do potenciômetro estão invertidas. Troque 3V3 e GND de lugar
- Leitura pula entre 0 e 4095: o fio do meio não está no cursor
- Erro de compilação `'ledcSetup' was not declared`: o código é do core 2.x. Os exemplos daqui usam o 3.x

---

## 5. OLED + BMP280 + LED — prática da Oficina 2

![Montagem completa da prática 2](../slides/assets/circuito-pratica-02.svg)

**Material:** display OLED SSD1306, sensor BMP280, 1 LED, 1 resistor de 220 Ω, jumpers.

É a junção do [circuito 3](#3-módulo-i2c--scanner) com o LED do [circuito 4](#4-led--buzzer--potenciômetro--pwm). Monte em etapas, como o roteiro da prática pede:

**Parte A — só o OLED.** Ligue os quatro fios do display e rode o `02a-scanner-i2c`. Tem que aparecer `0x3C`.

**Parte C — some o sensor.** Ligue o BMP280 nos **mesmos** quatro fios:

| Pino do BMP280 | Liga em |
|---|---|
| `VCC` | `3V3` |
| `GND` | `GND` |
| `SCL` | `GPIO 22` (junto com o SCL do OLED) |
| `SDA` | `GPIO 21` (junto com o SDA do OLED) |
| `CSB` | deixe solto (ou em `3V3`) |
| `SDO` | deixe solto, ou `GND` para `0x76` / `3V3` para `0x77` |

Rode o scanner de novo: agora tem que aparecer **dois** endereços, `0x3C` e `0x76`.

**Parte D — some o LED.** `GPIO 13` → resistor de 220 Ω → LED → `GND`.

```
                 ┌── OLED    VCC  GND  SDA  SCL
3V3  ────────────┤            │    │    │    │
GND  ────────────┤            │    │    │    │
GPIO 21 (SDA) ───┤  BMP280   VCC  GND  SDA  SCL
GPIO 22 (SCL) ───┘

GPIO 13 ──[ 220 Ω ]──►|── GND
```

Grave o `02c-oled-sensor-pwm`. No PlatformIO as bibliotecas vêm sozinhas; na Arduino IDE instale pelo Library Manager: **Adafruit SSD1306**, **Adafruit GFX** e **Adafruit BMP280**.

**Deu certo se** o display mostrar temperatura e pressão, e o LED ficar mais forte quando você segura o sensor com os dedos (a temperatura sobe alguns graus). Ajuste `TEMP_MIN` e `TEMP_MAX` para a temperatura da sala.

**Problemas comuns**
- `ERRO: display nao encontrado`: o endereço no código não é o do scanner. Troque `END_OLED`
- `ERRO: sensor nao encontrado`: troque `END_SENSOR` para `0x77`. Se o scanner mostrar `0x68` em vez de `0x76`, o seu módulo é um MPU6050, não um BMP280: veja a variante abaixo
- Display aceso mas vazio: faltou `display.display()` no seu código

---

## 5b. Variante: OLED + MPU6050 + servo

Para as bancadas que receberam **MPU6050** em vez de BMP280. Não há exemplo pronto: é a Parte D da prática, com a inclinação controlando o ângulo do servo.

**Material:** display OLED SSD1306, sensor MPU6050, servo SG90, jumpers.

**OLED e MPU6050:** mesmos quatro fios do [circuito 3](#3-módulo-i2c--scanner), no mesmo barramento. O MPU6050 responde em `0x68`. Os pinos `XDA`, `XCL`, `AD0` e `INT` do módulo ficam soltos.

**O servo:**

![Ligação do servo](../slides/assets/circuito-servo.svg)

| Fio do servo | Liga em |
|---|---|
| laranja ou amarelo (sinal) | `GPIO 26` |
| vermelho (alimentação) | **`VIN`** (5 V do USB) ou fonte externa de 5 V |
| marrom ou preto (terra) | `GND` |

> **O servo não vai no 3V3.** Ao se mover, ele puxa picos de centenas de miliampères que o regulador de 3,3 V não entrega. Se usar fonte externa, o GND dela **tem que ser ligado** ao GND da ESP32.

**Problema clássico:** a placa reinicia toda vez que o servo se mexe. É queda de tensão, não bug: troque de porta USB ou use fonte externa.

Biblioteca do servo: **ESP32Servo**.

---

## 6. Timer de hardware

**Material:** o mesmo LDR + resistor de 10 kΩ do [circuito 1](#1-ldr--led--leitura-analógica) **ou** o potenciômetro do [circuito 4](#4-led--buzzer--potenciômetro--pwm).

Ligue um dos dois no `GPIO 34`, exatamente como antes. O LED não é usado.

**Deu certo se** o monitor mostrar amostras numeradas (`#1`, `#2`, ...) e o contador avançar cerca de **10 por segundo**, mesmo com o `delay(250)` no `loop()`. Como o loop imprime só a última amostra, a numeração pula de 2 ou 3 em 3. Isso é esperado: é a prova de que quem amostra é o timer, não o loop.

---

## 7. Sensor analógico — tasks, fila e persistência

**Material:** o LDR + resistor de 10 kΩ do [circuito 1](#1-ldr--led--leitura-analógica) **ou** o potenciômetro do [circuito 4](#4-led--buzzer--potenciômetro--pwm).

É o mesmo sensor no `GPIO 34` do [circuito 6](#6-timer-de-hardware). Serve para dois exemplos:

**`03a-tasks-e-fila`** — duas tasks conversando por fila. **Deu certo se** aparecerem as linhas `tarefaSensor no nucleo 1` e `tarefaProcessa no nucleo 1`, seguidas de leituras com `na fila= 0`. Troque `CONSUMIDOR_LENTO_MS` para 2000: em cerca de 1 segundo `na fila` chega a 9 ou 10 e `descartadas` começa a subir.

**`03b-persistencia`** — Preferences e LittleFS. Antes de gravar na Arduino IDE, escolha *Ferramentas → Partition Scheme → Default 4MB with spiffs* (no PlatformIO isso já está no `platformio.ini`). **Deu certo se**, depois de tirar o cabo e ligar de novo, o `boot #` subir e `linhas ja gravadas` continuar crescendo. Digite `d` no monitor para ver o arquivo e `a` para apagar tudo.

**Problemas comuns**
- `ERRO: LittleFS nao montou`: o esquema de partição não tem espaço para arquivos. Troque e grave de novo. Trocar o esquema apaga o que estava na flash
- Leitura sempre 0 ou 4095: o mesmo diagnóstico do [circuito 1](#1-ldr--led--leitura-analógica)

---

## 8. Sensor + botão de despertar — deep sleep

**Material:** sensor do circuito 7, 1 push button, 1 resistor de 10 kΩ, jumpers.

Mantenha o sensor no `GPIO 34` e acrescente o botão no `GPIO 33`. Diferente do [circuito 2](#2-botão--led--interrupção), aqui o botão vai para o **3V3**, com um resistor de **pull-down** para o GND:

| De | Para |
|---|---|
| `GPIO 33` | uma perna do botão |
| perna diagonal oposta do botão | `3V3` |
| `GPIO 33` | resistor de 10 kΩ |
| outra perna do 10 kΩ | `GND` |

```
3V3 ──[ botão ]──┬── GPIO 33
                 │
              [ 10 kΩ ]
                 │
                GND
```

**Por que assim:** o código acorda a placa quando o pino vai a nível **alto** (`esp_sleep_enable_ext0_wakeup(GPIO_NUM_33, 1)`). O resistor segura o pino em LOW enquanto o botão está solto; sem ele, o pino fica flutuando e a placa acorda sozinha.

**Por que o GPIO 33:** só os RTC GPIO acordam a placa: `0, 2, 4, 12–15, 25–27, 32–39`. O GPIO 4 do circuito 2 também é RTC, mas o 33 deixa os pinos de boot em paz.

**Deu certo se** o monitor mostrar um ciclo a cada 15 s, com `acordei por: timer`, e apertar o botão durante o sono acordar na hora, com `acordei por: botao (ext0)`.

**Problemas comuns**
- Acorda sem parar, sem ninguém apertar: falta o resistor de pull-down, ou o botão está nas pernas do mesmo par (sempre fechado)
- O contador de boots volta a 1: você apertou o **EN** (reset) ou tirou da tomada. `RTC_DATA_ATTR` sobrevive ao sono, não ao reset

---

## 9. Data logger completo — prática da Oficina 3

![Montagem completa da prática 3](../slides/assets/circuito-pratica-03.svg)

**Material:** sensor e botão do [circuito 8](#8-sensor--botão-de-despertar--deep-sleep), mais o display OLED.

É o circuito 8 com o OLED do [circuito 3](#3-módulo-i2c--scanner) por cima: `VCC → 3V3`, `GND → GND`, `SDA → GPIO 21`, `SCL → GPIO 22`. O display é **opcional**: sem ele, o `03d-datalogger-completo` avisa no monitor e segue só pelo serial.

Na Arduino IDE, instale **Adafruit SSD1306** e **Adafruit GFX** e escolha o esquema de partição com SPIFFS, como no circuito 7.

**Deu certo se** a cada ciclo de 10 s:
1. o display mostrar `boot #N` e o motivo do despertar (`reset`, `timer` ou `botao`)
2. as 20 leituras do ciclo passarem na tela, com `linhas no arquivo` subindo
3. aparecer `dormindo` e a tela congelar até o próximo ciclo

Tire o cabo e ligue de novo: `boot #` volta a 1 (era `RTC_DATA_ATTR`), mas `linhas no arquivo` continua de onde parou (é LittleFS).

---

## Medindo o consumo

**Material:** multímetro, jumpers. Use o circuito 8 e o `03c-deep-sleep`.

O multímetro vai **em série** com a alimentação, na escala de corrente: o fio que sai da fonte entra no multímetro, e o que sai do multímetro vai ao `VIN` (5 V) da placa. O GND da fonte vai direto ao GND da placa. Comece pela escala de **mA**: na hora de acordar a placa puxa picos que podem queimar o fusível da escala de µA.

```
fonte 5 V (+) ──[ multímetro em mA ]── VIN
fonte 5 V (−) ───────────────────────── GND
```

> **Não espere ver 10 µA numa DevKit.** O regulador de tensão, o LED de alimentação e o chip USB-serial da placa continuam ligados no deep sleep e consomem alguns mA sozinhos. Os ~10 µA da tabela dos slides são do **chip**; para chegar perto disso, o projeto final precisa de uma placa sem esses extras ou de um módulo ESP32 alimentado direto.

Mesmo assim, a diferença entre acordado (dezenas de mA) e dormindo aparece claramente no visor.

---

## 10. Sensor + LED na rede — prática da Oficina 4

![Montagem completa da prática 4](../slides/assets/circuito-pratica-04.svg)

**Material:** o mesmo do [circuito 1](#1-ldr--led--leitura-analógica): LDR com resistor de 10 kΩ no `GPIO 34` (ou o potenciômetro) e LED com resistor de 220 Ω no `GPIO 13`. O display não é usado.

A montagem é a mais simples da série; o trabalho desta oficina está na rede. Antes de gravar, **troque as constantes no topo de cada exemplo**:

| Constante | O que pôr | Exemplos |
|---|---|---|
| `SSID`, `SENHA` | a rede do roteador da oficina (o eduroam **não** serve) | todos |
| `BROKER` | o IP do notebook que roda o Mosquitto | `04c`, `04d` |
| `BANCADA` | um nome só seu, como `b1`, `b2`... | `04c`, `04d` |
| `VIZINHA` | o `BANCADA` de outra equipe | `04d` |

Na Arduino IDE, instale **PubSubClient** (de Nick O'Leary) para o `04c` e o `04d`. No PlatformIO ela já está no `platformio.ini`.

**`04a-wifi-conexao`** — **deu certo se** aparecer `conectado!  IP: ...` e, a cada 2 s, uma linha `ok | IP ... | RSSI ...`. Desligue o roteador: aparece `caiu! motivo 200` (ou 201) e, quando ele volta, `conectado!` de novo, sem apertar reset.

**`04b-servidor-web`** — abra no celular o endereço que o monitor mostrar (`http://192.168.x.y`, ou `http://peteco.local`). **Deu certo se** o número na tela mudar ao cobrir o LDR, sem recarregar a página, e o botão acender e apagar o LED.

**`04c-mqtt`** — no notebook do broker, rode `mosquitto_sub -h localhost -t 'peteco/#' -v`. **Deu certo se** aparecer `peteco/b1/status online` e uma linha `peteco/b1/sensor {...}` a cada 5 s. Em outro terminal, `mosquitto_pub -h localhost -t 'peteco/b1/led' -m 1` acende o LED.

**`04d-painel-completo`** — junta tudo. **Deu certo se** a página mostrar a sua leitura, a da bancada vizinha (quando ela estiver publicando) e a bolinha do MQTT verde. O botão da página acende o LED e publica a mudança em `peteco/b1/led/estado`.

**Problemas comuns**
- Fica só imprimindo pontos: SSID ou senha errados, ou a rede é de 5 GHz. A ESP32 só enxerga redes de **2,4 GHz**
- Motivo 15 ou 202 logo ao ligar: senha errada
- A página não abre no celular: o celular está em outra rede ou nos dados móveis. Digite `http://`, não `https://`
- `peteco.local` não abre: no Android e no Windows sem Bonjour o mDNS costuma falhar. Use o IP
- MQTT `falhou (estado -2)`: IP do broker errado, ou o firewall do notebook bloqueando a porta 1883. O Mosquitto 2.x só aceita conexões de fora com `listener 1883` e `allow_anonymous true` no `mosquitto.conf`
- Conecta no broker e cai em segundos, sem parar: duas placas com o mesmo client ID. Os exemplos montam o ID com o MAC justamente para evitar isso; acontece quando se copia o código sem essa parte

---

## Mapa de pinos usado nas oficinas

| GPIO | Uso | Circuitos |
|---|---|---|
| 4 | botão (entrada com pull-up interno) | 2 |
| 13 | LED (PWM) | 1, 2, 4, 5, 10 |
| 21 | I2C SDA | 3, 5, 5b, 9 |
| 22 | I2C SCL | 3, 5, 5b, 9 |
| 25 | buzzer passivo (PWM) | 4 |
| 26 | servo (PWM) | 5b |
| 33 | botão de despertar, com pull-down externo (RTC GPIO) | 8, 9 |
| 34 | entrada analógica, ADC1 (LDR ou potenciômetro) | 1, 4, 6, 7, 8, 9, 10 |

Todos estão na lista de pinos seguros dos slides. Se precisar de outro para o projeto de vocês, escolha também nessa lista: `13 14 16 17 18 19 21 22 23 25 26 27 32 33` para entrada e saída, `34 35 36 39` só para entrada.
