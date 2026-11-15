"""
Gera o diagrama de ligação da Oficina 5 em assets/, com Schemdraw.

    python3 -m venv .venv && .venv/bin/pip install schemdraw
    .venv/bin/python scripts/circuitos_oficina5.py

Os pinos e as ligações seguem exemplos/MONTAGEM.md. Mudou um pino lá ou no
código? Mude aqui também e rode de novo.
"""

import schemdraw
import schemdraw.elements as elm

from circuitos_comum import AMARELO, ASSETS, esp32, fio, texto


# --------------------------------------------------------------------------
# Nó ESP-NOW de cada bancada: sensor no ADC1 (vai para o gateway), LED
# (comando do gateway e pulso da vizinha) e botão (manda o pulso)
#    — slide "A montagem de cada bancada" da Oficina 5
# --------------------------------------------------------------------------
def pratica():
    with schemdraw.Drawing(show=False) as d:
        e = d.add(esp32([("GPIO 34", "adc"), ("GPIO 13", "led"),
                         ("GPIO 4", "btn")], 4.4))

        # LED: acende pelo comando do gateway e pisca com o pulso da vizinha
        d.add(elm.Line().at(e.led).right(1.5))
        d.add(elm.Resistor().right().label("220 Ω", loc="top"))
        d.add(elm.LED().right().label("LED", loc="bottom").fill(AMARELO))
        d.add(elm.Line().right(0.6))
        d.add(elm.Ground())

        # Botão para o GND, com o pull-up interno: curto, para o terra dele
        # não encostar na linha do LED acima
        d.add(elm.Line().at(e.btn).right(1.5))
        d.add(elm.Button().right().label("botão\n(pulso)", loc="bottom", ofst=0.3))
        d.add(elm.Line().right(0.4))
        d.add(elm.Ground())

        # Divisor de tensão do LDR no GPIO 34 (ADC1), à direita de tudo
        na = (e.adc.x + 11, e.adc.y)
        fio(d, e.adc, na)
        d.add(elm.Dot(radius=0.12).at(na))
        d.add(elm.Photoresistor().at(na).up().label("LDR", loc="bottom", ofst=0.3)
              .fill(AMARELO))
        d.add(elm.Vdd().label("3V3"))
        d.add(elm.Resistor().at(na).down().label("10 kΩ", loc="bottom", ofst=0.3))
        d.add(elm.Ground())

        texto(d, (e.adc.x + 7.5, e.btn.y - 3.0),
              "O botão usa o pull-up interno: liga o GPIO 4 direto ao GND, sem resistor.",
              alinha="center")
        texto(d, (e.adc.x + 7.5, e.btn.y - 3.7),
              "O gateway (placa do professor) não tem montagem: é só a placa no USB.",
              alinha="center")
        d.save(ASSETS / "circuito-pratica-05.svg")


if __name__ == "__main__":
    pratica()
    print("gerado em", ASSETS)
