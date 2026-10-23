"""
Gera o diagrama de ligação da Oficina 3 em assets/, com Schemdraw.

    python3 -m venv .venv && .venv/bin/pip install schemdraw
    .venv/bin/python scripts/circuitos_oficina3.py

Os pinos e as ligações seguem exemplos/MONTAGEM.md. Mudou um pino lá ou no
código? Mude aqui também e rode de novo.
"""

import schemdraw
import schemdraw.elements as elm

from circuitos_comum import AMARELO, ASSETS, CINZA, bloco, esp32, fio, texto


# --------------------------------------------------------------------------
# Data logger: sensor no ADC1, OLED no I2C e botão de despertar com pull-down
#    — slide "A montagem completa" da Oficina 3
# --------------------------------------------------------------------------
def pratica():
    with schemdraw.Drawing(show=False) as d:
        e = d.add(esp32([("3V3", "v"), ("GND", "g"), ("GPIO 21", "sda"),
                         ("GPIO 22", "scl"), ("GPIO 33", "btn"),
                         ("GPIO 34", "adc")], 7.0))

        # Barramento I2C com o OLED por cima, como na Oficina 2
        fim = e.v.x + 9
        nomes = {"v": "3,3 V", "g": "GND", "sda": "SDA", "scl": "SCL"}
        for a in nomes:
            p = e.absanchors[a]
            fio(d, p, (fim, p.y))
            texto(d, (fim + 0.2, p.y), nomes[a], cor=CINZA, tam=12)

        m = d.add(
            bloco("Display OLED\nSSD1306 (0x3C)\nopcional", "bottom",
                  [("VCC", "v"), ("GND", "g"), ("SDA", "sda"), ("SCL", "scl")],
                  (5.0, 2.6))
            .anchor("v").at((e.v.x + 2.2, e.v.y + 1.6))
        )
        for a in nomes:
            pino = m.absanchors[a]
            fio(d, pino, (pino.x, e.absanchors[a].y))
            d.add(elm.Dot(radius=0.12).at((pino.x, e.absanchors[a].y)))

        # Botão no GPIO 33 (RTC GPIO): sobe para o 3V3, pull-down para o GND.
        # Fica para a direita do fim do barramento, para não cruzar fios.
        nb = (e.v.x + 12, e.btn.y)
        fio(d, e.btn, nb)
        d.add(elm.Dot(radius=0.12).at(nb))
        d.add(elm.Button().at(nb).up().label("botão\n(acorda)", loc="bottom",
                                             ofst=0.3))
        d.add(elm.Vdd().label("3V3"))
        d.add(elm.Resistor().at(nb).right().label("10 kΩ\npull-down", loc="top"))
        d.add(elm.Ground())

        # Divisor de tensão do LDR no GPIO 34 (ADC1)
        na = (e.v.x + 18.5, e.adc.y)
        fio(d, e.adc, na)
        d.add(elm.Dot(radius=0.12).at(na))
        d.add(elm.Photoresistor().at(na).up().label("LDR", loc="bottom", ofst=0.3)
              .fill(AMARELO))
        d.add(elm.Vdd().label("3V3"))
        d.add(elm.Resistor().at(na).down().label("10 kΩ", loc="bottom", ofst=0.3))
        d.add(elm.Ground())

        texto(d, (e.v.x + 10, e.adc.y - 4.0),
              "No lugar do LDR serve um potenciômetro: pernas externas no 3V3 e no GND, "
              "cursor no GPIO 34.",
              alinha="center")
        texto(d, (e.v.x + 10, e.adc.y - 4.7),
              "Sem o OLED o programa segue só pelo monitor serial.",
              alinha="center")
        d.save(ASSETS / "circuito-pratica-03.svg")


if __name__ == "__main__":
    pratica()
    print("gerado em", ASSETS)
