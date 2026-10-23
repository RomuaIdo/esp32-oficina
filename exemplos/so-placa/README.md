# Exemplos só com a placa

Demonstrações que rodam numa ESP32 DevKit **sem nenhum componente externo**. Usam só o que já vem na placa:

| Recurso | Onde |
|---|---|
| LED azul | GPIO 2 |
| Botão **BOOT** | GPIO 0 (já tem pull-up na placa) |
| Sensor de toque capacitivo | GPIO 4, marcado **D4**: basta encostar o dedo no pino |
| Wi-Fi | o rádio da própria placa |

| Exemplo | O que mostra | Tema |
|---|---|---|
| `a-pisca-sem-delay` | LED piscando com `millis()`, e o loop contando voltas para provar que nunca para | Oficina 1 |
| `b-botao-boot` | BOOT por interrupção alternando o LED, com debounce sem `delay()` | Oficina 1 |
| `c-toque` | Toque no D4 acende o LED; a curva aparece no Serial Plotter / Teleplot | Oficina 1 |
| `d-fade-pwm` | LED "respirando" por PWM (LEDC, API 3.x); BOOT troca a velocidade | Oficina 2 |
| `e-tasks-e-fila` | ISR do BOOT → fila → task de botão → fila → task que pisca o LED | Oficina 3 |
| `f-deep-sleep` | Dorme 10 s ou até tocar no D4; contador em RTC RAM x contador no Preferences | Oficina 3 |
| `g-scanner-wifi` | Lista as redes ao redor com RSSI, canal e segurança | Prévia da Oficina 4 |
| `h-servidor-web` | A placa cria a própria rede Wi-Fi e serve uma página que liga e desliga o LED pelo celular | Prévia da Oficina 4 |

> **A placa não tem LED azul?** Algumas DevKitC só têm o LED vermelho de alimentação. Os exemplos continuam funcionando pelo monitor serial; para ver o LED, ligue um LED com resistor de 220 Ω no GPIO 2.

---

## Gravar na hora — PlatformIO (terminal)

Da raiz do repositório, troque o nome do exemplo no `cd`:

```bash
cd exemplos/so-placa/platformio/a-pisca-sem-delay
pio run -t upload -t monitor
```

Isso compila, grava e já abre o monitor serial a 115200. `Ctrl+C` fecha o monitor. Para só compilar, sem placa conectada: `pio run`.

Para compilar **todos** de uma vez (bom para conferir antes da apresentação):

```bash
for d in exemplos/so-placa/platformio/*/; do (cd "$d" && pio run) || echo "FALHOU: $d"; done
```

Se houver mais de uma porta serial, indique qual: `pio run -t upload -t monitor --upload-port /dev/ttyUSB0 --monitor-port /dev/ttyUSB0` (no Windows, `COM3` etc.).

> **A primeira compilação baixa a plataforma** (alguns GB) e leva vários minutos. Faça isso antes da apresentação. As seguintes levam uns 10 segundos.

## Gravar na hora — Arduino IDE

Abra o `.ino` da pasta do exemplo, selecione *ESP32 Dev Module* e a porta, e clique em *Carregar*. O `f-deep-sleep` e os outros não precisam de bibliotecas extras nem de mudar o esquema de partição.

---

## Roteiro de demonstração sugerido

1. **`a-pisca-sem-delay`**: aponte o número de voltas por segundo no monitor. Pergunte quanto seria com `delay(500)`
2. **`b-botao-boot`**: aperte o BOOT algumas vezes. Bom gancho para falar de repique
3. **`c-toque`**: abra o plotter e encoste no D4. A queda da curva é visível do fundo da sala
4. **`d-fade-pwm`**: apague a luz da sala; o efeito do duty cycle fica óbvio
5. **`e-tasks-e-fila`**: aperte o BOOT e mostre que cada task imprime de um lado, sem variável global
6. **`f-deep-sleep`**: deixe dormir dois ciclos, toque no D4, aperte EN, tire da tomada. Compare os dois contadores
7. **`h-servidor-web`**: projete o monitor serial com o nome da rede e peça para a turma conectar. Todo mundo liga e desliga o mesmo LED

> No `f-deep-sleep`, o monitor serial pode perder a primeira linha de cada ciclo enquanto reconecta. O `delay(300)` no início ajuda, mas não é garantia.
