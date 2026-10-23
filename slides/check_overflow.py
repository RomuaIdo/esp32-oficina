#!/usr/bin/env python3
"""
Mede a altura real do conteúdo de cada slide Marp.

O Marp não avisa quando o conteúdo passa da área útil do slide: ele
simplesmente corta. Este script gera o HTML com o próprio Marp, abre no
Chrome headless (o mesmo que o Marp usa para gerar PDF) e mede a caixa
ocupada pelo conteúdo de cada slide, com as fontes, imagens e o tema reais.

Uso:  python3 check_overflow.py arquivo.md [outro.md ...]

Precisa de Node.js (para o npx do Marp) e de Chrome/Chromium/Edge. Se o
navegador não for encontrado sozinho, aponte com a variável CHROME_PATH.
"""
import html
import json
import os
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

RAIZ = Path(__file__).resolve().parent

# Mesmas opções do Makefile, para medir exatamente o que vai para o PDF.
MARP_FLAGS = ["--no-stdin", "--theme-set", "themes/", "--allow-local-files"]

NAVEGADORES = [
    "google-chrome-stable", "google-chrome", "chromium", "chromium-browser",
    "microsoft-edge", "microsoft-edge-stable", "brave",
    "/Applications/Google Chrome.app/Contents/MacOS/Google Chrome",
    r"C:\Program Files\Google\Chrome\Application\chrome.exe",
    r"C:\Program Files (x86)\Microsoft\Edge\Application\msedge.exe",
]

LIMITE_ALERTA = 90   # % da área útil a partir do qual vale olhar o slide

# Roda dentro da página: para cada <section>, pega a caixa que envolve todo o
# conteúdo (sem header, footer e paginação, que ficam fora da área útil) e
# compara com a área entre os paddings do slide. Funciona também nos slides
# centralizados, em que o conteúdo transborda para cima e para baixo.
MEDIDOR = r"""
<script>
window.addEventListener('load', async () => {
  await document.fonts.ready;
  const res = [];
  document.querySelectorAll('section[id]').forEach((sec) => {
    const caixa = sec.getBoundingClientRect();
    const escala = caixa.height / sec.offsetHeight || 1;
    const css = getComputedStyle(sec);
    const topo = parseFloat(css.paddingTop);
    const base = parseFloat(css.paddingBottom);
    const util = sec.offsetHeight - topo - base;

    let minY = Infinity, maxY = -Infinity;
    for (const el of sec.children) {
      if (/^(HEADER|FOOTER|STYLE|SCRIPT)$/.test(el.tagName)) continue;
      const r = el.getBoundingClientRect();
      if (r.height === 0 && r.width === 0) continue;
      const m = getComputedStyle(el);
      minY = Math.min(minY, (r.top - caixa.top) / escala - parseFloat(m.marginTop));
      maxY = Math.max(maxY, (r.bottom - caixa.top) / escala + parseFloat(m.marginBottom));
    }
    const h1 = sec.querySelector('h1, h2, h3');
    res.push({
      n: Number(sec.id),
      classe: sec.getAttribute('data-class') || '',
      titulo: h1 ? h1.textContent.trim() : '(sem título)',
      usado: isFinite(maxY) ? maxY - minY : 0,
      util: util,
      cortaCima: isFinite(minY) && minY < -0.5,
      cortaBaixo: isFinite(maxY) && maxY > sec.offsetHeight + 0.5,
      invadeRodape: isFinite(maxY) && maxY > sec.offsetHeight - base + 0.5,
    });
  });
  const pre = document.createElement('pre');
  pre.id = 'medidas-overflow';
  pre.textContent = JSON.stringify(res);
  document.body.appendChild(pre);
});
</script>
"""


def acha_navegador():
    candidato = os.environ.get("CHROME_PATH")
    if candidato:
        return candidato
    for nome in NAVEGADORES:
        caminho = shutil.which(nome) or (nome if Path(nome).is_file() else None)
        if caminho:
            return caminho
    sys.exit("ERRO: Chrome/Chromium/Edge não encontrado. Defina CHROME_PATH.")


def comando_marp():
    local = RAIZ / "node_modules" / ".bin" / "marp"
    if local.exists():
        return [str(local)]
    if shutil.which("marp"):                 # instalado com npm -g
        return ["marp"]
    if not shutil.which("npx"):
        sys.exit("ERRO: npx não encontrado. Instale o Node.js 18+.")
    return ["npx", "--yes", "@marp-team/marp-cli@latest"]


def gera_html(md, destino):
    cmd = comando_marp() + MARP_FLAGS + ["--template", "bare", str(md), "-o", str(destino)]
    r = subprocess.run(cmd, cwd=RAIZ, capture_output=True, text=True)
    if r.returncode != 0:
        sys.exit(f"ERRO ao rodar o Marp:\n{r.stderr}")


def mede(md, navegador):
    with tempfile.TemporaryDirectory() as tmp:
        pagina = Path(tmp) / "slides.html"
        gera_html(md, pagina)

        # <base> faz os caminhos relativos (assets/...) resolverem na raiz
        base = f'<base href="{(md.parent.resolve()).as_uri()}/">'
        conteudo = pagina.read_text(encoding="utf-8")
        conteudo = conteudo.replace("<head>", "<head>" + base, 1)
        conteudo = conteudo.replace("</body>", MEDIDOR + "</body>", 1)
        pagina.write_text(conteudo, encoding="utf-8")

        cmd = [
            navegador, "--headless=new", "--disable-gpu", "--no-sandbox",
            "--allow-file-access-from-files", "--hide-scrollbars",
            "--window-size=1280,720", "--virtual-time-budget=15000",
            f"--user-data-dir={Path(tmp) / 'perfil'}",
            "--dump-dom", pagina.as_uri(),
        ]
        r = subprocess.run(cmd, capture_output=True, text=True, timeout=180)

    marca_ini = '<pre id="medidas-overflow">'
    if marca_ini not in r.stdout:
        sys.exit(f"ERRO: o navegador não devolveu as medidas.\n{r.stderr[-2000:]}")
    bruto = r.stdout.split(marca_ini, 1)[1].split("</pre>", 1)[0]
    return json.loads(html.unescape(bruto))


def relatorio(md, medidas):
    problemas = 0
    for s in medidas:
        pct = s["usado"] / s["util"] * 100 if s["util"] else 0
        if s["cortaCima"] or s["cortaBaixo"]:
            marca, problemas = "CORTADO  ", problemas + 1
        elif s["invadeRodape"] or pct > 100:
            marca, problemas = "ESTOURA  ", problemas + 1
        elif pct > LIMITE_ALERTA:
            marca = "no limite"
        else:
            continue
        print(f"  [{marca}] slide {s['n']:>2}  {pct:5.0f}%  {s['titulo'][:52]}")

    print(f"\n{len(medidas)} slides · {problemas} estourando a área útil")
    return problemas


def main(arquivos):
    navegador = acha_navegador()
    total = 0
    for nome in arquivos:
        md = Path(nome)
        if len(arquivos) > 1:
            print(f">> {md}")
        total += relatorio(md, mede(md, navegador))
    return total


if __name__ == "__main__":
    if len(sys.argv) < 2:
        sys.exit(__doc__)
    sys.exit(0 if main(sys.argv[1:]) == 0 else 1)
