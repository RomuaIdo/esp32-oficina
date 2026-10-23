"""
Gera os diagramas de ligação da Oficina 2 em assets/, com Schemdraw.

    python3 -m venv .venv && .venv/bin/pip install schemdraw
    .venv/bin/python scripts/circuitos_oficina2.py

Os pinos e as ligações seguem exemplos/MONTAGEM.md. Mudou um pino lá ou no
código? Mude aqui também e rode de novo.
"""

import schemdraw
import schemdraw.elements as elm

from circuitos_comum import (AMARELO, ASSETS, CINZA, FIO_GND, FIO_SINAL, FIO_VCC,
                             PRETO, bloco, esp32, fio, texto)


# --------------------------------------------------------------------------
# 1. Display OLED no I2C — slide "Ligando o display na bancada"
# --------------------------------------------------------------------------
def oled():
    with schemdraw.Drawing(show=False) as d:
        e = d.add(esp32([("3V3", "v"), ("GND", "g"), ("GPIO 21", "sda"),
                         ("GPIO 22", "scl")], 5.0))
        o = d.add(
            bloco("Display OLED\nSSD1306\n(0x3C)", "left",
                  [("VCC", "v"), ("GND", "g"), ("SDA", "sda"), ("SCL", "scl")],
                  (4.4, 5.0))
            .anchor("v").at((e.v.x + 6.5, e.v.y))
        )
        for a, r in [("v", "3,3 V"), ("g", "GND"), ("sda", "SDA (dados)"),
                     ("scl", "SCL (clock)")]:
            fio(d, e.absanchors[a], o.absanchors[a], rotulo=r)

        texto(d, (e.v.x + 3.25, e.scl.y - 1.6),
              "Confira a serigrafia: a ordem dos pinos muda entre módulos.",
              alinha="center")
        d.save(ASSETS / "circuito-oled-i2c.svg")


# --------------------------------------------------------------------------
# 2. Servo SG90 — slide "Ligação do servo"
# --------------------------------------------------------------------------
def servo():
    with schemdraw.Drawing(show=False) as d:
        e = d.add(esp32([("GPIO 26", "sig"), ("VIN (5 V)", "v"), ("GND", "g")], 4.0))
        s = d.add(
            bloco("Servo\nSG90", "left",
                  [("sinal", "sig"), ("+5 V", "v"), ("GND", "g")], (3.6, 4.0))
            .anchor("sig").at((e.sig.x + 6.5, e.sig.y))
        )
        fio(d, e.sig, s.sig, FIO_SINAL, "laranja · sinal PWM")
        fio(d, e.v, s.v, FIO_VCC, "vermelho · 5 V")
        fio(d, e.g, s.g, FIO_GND, "marrom · GND")

        x = (e.g.x + s.g.x) / 2
        texto(d, (x, e.g.y - 1.6),
              "Nunca no 3V3: o servo puxa picos de centenas de mA.",
              cor=PRETO, alinha="center")
        texto(d, (x, e.g.y - 2.25),
              "Com fonte externa de 5 V, ligue o GND dela ao GND da ESP32.",
              alinha="center")
        d.save(ASSETS / "circuito-servo.svg")


# --------------------------------------------------------------------------
# 3. Prática completa: OLED + BMP280 no mesmo barramento + LED por PWM
#    — slide "A montagem completa"
# --------------------------------------------------------------------------
def pratica():
    with schemdraw.Drawing(show=False) as d:
        e = d.add(esp32([("3V3", "v"), ("GND", "g"), ("GPIO 21", "sda"),
                         ("GPIO 22", "scl"), ("GPIO 13", "led")], 6.0))

        # Barramento: quatro linhas horizontais saindo da placa
        fim = e.v.x + 15
        nomes = {"v": "3,3 V", "g": "GND", "sda": "SDA", "scl": "SCL"}
        for a in nomes:
            p = e.absanchors[a]
            fio(d, p, (fim, p.y))
            texto(d, (fim + 0.2, p.y), nomes[a], cor=CINZA, tam=12)

        # Módulos acima do barramento, pinos embaixo, descendo até cada linha
        topo = e.v.y + 1.6
        for x0, titulo in [(e.v.x + 2.2, "Display OLED\nSSD1306 (0x3C)"),
                           (e.v.x + 8.4, "Sensor BMP280\n(0x76)")]:
            m = d.add(
                bloco(titulo, "bottom",
                      [("VCC", "v"), ("GND", "g"), ("SDA", "sda"), ("SCL", "scl")],
                      (5.0, 2.4))
                .anchor("v").at((x0, topo))
            )
            for a in nomes:
                pino = m.absanchors[a]
                linha_y = e.absanchors[a].y
                fio(d, pino, (pino.x, linha_y))
                d.add(elm.Dot(radius=0.12).at((pino.x, linha_y)))

        # LED por PWM abaixo do barramento
        d.add(elm.Line().at(e.led).right(1.5))
        d.add(elm.Resistor().right().label("220 Ω", loc="bottom"))
        d.add(elm.LED().right().label("LED", loc="bottom").fill(AMARELO))
        d.add(elm.Line().right(0.6))
        d.add(elm.Ground())

        texto(d, (e.v.x + 7.5, e.led.y - 1.9),
              "Os dois módulos dividem os mesmos quatro fios: o endereço diferencia quem responde.",
              alinha="center")
        d.save(ASSETS / "circuito-pratica-02.svg")


if __name__ == "__main__":
    oled()
    servo()
    pratica()
    print("gerados em", ASSETS)
