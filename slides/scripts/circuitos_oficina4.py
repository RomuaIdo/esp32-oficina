"""
Gera o diagrama de ligação da Oficina 4 em assets/, com Schemdraw.

    python3 -m venv .venv && .venv/bin/pip install schemdraw
    .venv/bin/python scripts/circuitos_oficina4.py

Os pinos e as ligações seguem exemplos/MONTAGEM.md. Mudou um pino lá ou no
código? Mude aqui também e rode de novo.
"""

import schemdraw
import schemdraw.elements as elm

from circuitos_comum import AMARELO, ASSETS, esp32, fio, texto


# --------------------------------------------------------------------------
# Painel IoT: sensor no ADC1 (vai para a página e para o MQTT) e LED
# comandado pela rede — slide "A montagem completa" da Oficina 4
# --------------------------------------------------------------------------
def pratica():
    with schemdraw.Drawing(show=False) as d:
        e = d.add(esp32([("GPIO 34", "adc"), ("GPIO 13", "led")], 3.4))

        # LED comandado pela página e pelo tópico .../led
        d.add(elm.Line().at(e.led).right(1.5))
        d.add(elm.Resistor().right().label("220 Ω", loc="bottom"))
        d.add(elm.LED().right().label("LED", loc="bottom").fill(AMARELO))
        d.add(elm.Line().right(0.6))
        d.add(elm.Ground())

        # Divisor de tensão do LDR no GPIO 34 (ADC1). Fica à direita do fim
        # da linha do LED, para o resistor descer sem cruzar fio nenhum.
        na = (e.adc.x + 11, e.adc.y)
        fio(d, e.adc, na)
        d.add(elm.Dot(radius=0.12).at(na))
        d.add(elm.Photoresistor().at(na).up().label("LDR", loc="bottom", ofst=0.3)
              .fill(AMARELO))
        d.add(elm.Vdd().label("3V3"))
        d.add(elm.Resistor().at(na).down().label("10 kΩ", loc="bottom", ofst=0.3))
        d.add(elm.Ground())

        # A outra metade da montagem é a rede, que não tem fio: só legenda
        texto(d, (e.adc.x + 7.5, e.led.y - 3.6),
              "No lugar do LDR serve um potenciômetro: pernas externas no 3V3 e no GND, "
              "cursor no GPIO 34.",
              alinha="center")
        texto(d, (e.adc.x + 7.5, e.led.y - 4.3),
              "O resto é sem fio: a placa entra no roteador da oficina, serve a página "
              "e fala com o broker MQTT.",
              alinha="center")
        d.save(ASSETS / "circuito-pratica-04.svg")


if __name__ == "__main__":
    pratica()
    print("gerado em", ASSETS)
