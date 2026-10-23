"""
Peças comuns aos diagramas de circuito das oficinas (Schemdraw).

Os scripts circuitos_oficinaN.py importam daqui: cores do tema, o bloco da
ESP32, blocos de módulo, fios e textos.
"""

from pathlib import Path

import schemdraw
import schemdraw.elements as elm

ASSETS = Path(__file__).resolve().parent.parent / "assets"

# Cores do tema (themes/peteco.css)
AMARELO = "#f5c400"
PRETO = "#14161a"
CINZA = "#5b6270"
MODULO = "#eef0f3"

# Cores reais dos fios do servo SG90
FIO_SINAL = "#e8820c"
FIO_VCC = "#d62828"
FIO_GND = "#6b3e1f"

schemdraw.config(
    font="Inter, Segoe UI, Helvetica Neue, Arial, sans-serif",
    fontsize=13,
    bgcolor="white",
    color=PRETO,
    lw=2,
)


def bloco(titulo, lado, pinos, tamanho, cor=MODULO, fonte=14):
    """
    Bloco retangular com pinos num lado só.

    pinos: lista de (rótulo, âncora), na ordem de leitura — de cima para baixo
    nos lados, da esquerda para a direita em cima/embaixo.
    """
    # Afasta o título dos rótulos dos pinos, para o lado oposto a eles
    desvio = {"left": (0.7, 0), "right": (-0.7, 0), "bottom": (0, 0.35)}[lado]
    if lado in ("left", "right"):
        pinos = list(reversed(pinos))  # o Ic numera os lados de baixo para cima
    return (
        elm.Ic(
            pins=[elm.IcPin(name=r, side=lado, anchorname=a) for r, a in pinos],
            pinspacing=1.0,
            edgepadH=0.8,
            edgepadW=0.8,
            size=tamanho,
            plblsize=13,
            lsize=13,
        )
        .theta(0)  # senão herda a direção do último fio desenhado
        .fill(cor)
        .label(titulo, loc="center", fontsize=fonte, ofst=desvio)
    )


def esp32(pinos, altura):
    return bloco("ESP32\nDevKit", "right", pinos, (4.2, altura), cor=AMARELO, fonte=17)


def fio(d, a, b, cor=PRETO, rotulo=None):
    """Fio reto entre dois pontos (mesma altura ou mesma coluna)."""
    w = elm.Line().at(a).to(b).color(cor).linewidth(2.5 if cor != PRETO else 2)
    if rotulo:
        w.label(rotulo, loc="top", fontsize=12, color=CINZA)
    d.add(w)


def texto(d, xy, msg, cor=CINZA, tam=12, alinha="left"):
    d.add(elm.Label().at(xy).label(msg, color=cor, fontsize=tam, halign=alinha))
