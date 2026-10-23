# Slides das oficinas

Escritos em [Marp](https://marp.app/) (Markdown → HTML/PDF/PPTX). **Todos os comandos abaixo rodam de dentro desta pasta** (`cd slides`).

```
slides/
├── 00-template.md               ← copie este para criar uma oficina nova
├── 01-fundamentos-esp32.md
├── 02-pwm-e-protocolos.md
├── 03-freertos-flash-energia.md
├── 04-wifi-e-iot.md
├── preparacao-oficina-04.md     ← o que o instrutor prepara antes da Oficina 4
├── themes/peteco.css            ← tema visual do grupo (preto + amarelo)
├── assets/                      ← imagens e diagramas
├── scripts/                     ← geradores dos diagramas de circuito (Schemdraw)
├── check_overflow.py            ← mede quais slides estouram a altura
├── package.json                 ← scripts npm
└── Makefile                     ← alternativa ao npm
```

O código das práticas fica em [`../exemplos/`](../exemplos/).

---

## Como compilar

### Opção 1 — sem instalar nada (`npx`)

Precisa apenas de [Node.js](https://nodejs.org/) 18 ou mais novo.

```bash
# HTML (abre em qualquer navegador)
mkdir -p build && cp -r assets build/assets      # o HTML usa caminho relativo
npx --yes @marp-team/marp-cli@latest --no-stdin --theme-set themes/ --allow-local-files \
    01-fundamentos-esp32.md -o build/01-fundamentos-esp32.html

# PDF, com as notas do apresentador anexadas
npx --yes @marp-team/marp-cli@latest --no-stdin --theme-set themes/ --allow-local-files \
    --pdf --pdf-notes --pdf-outlines 01-fundamentos-esp32.md -o build/01-fundamentos-esp32.pdf

# PowerPoint (útil se alguém do grupo for apresentar sem Node instalado)
npx --yes @marp-team/marp-cli@latest --no-stdin --theme-set themes/ --allow-local-files \
    --pptx 01-fundamentos-esp32.md -o build/01-fundamentos-esp32.pptx
```

### Opção 2 — com `npm`

```bash
npm install
npm run html     # HTML
npm run pdf      # PDF com notas
npm run pptx     # PowerPoint
npm run watch    # recompila e recarrega enquanto você edita
npm run check    # mede quais slides estouram a altura
```

### Opção 3 — `make`

```bash
make          # HTML de todas as oficinas
make pdf      # PDF de todas
make watch    # modo ao vivo
make clean
```

### Opção 4 — extensão do VS Code

Instale **Marp for VS Code** (`marp-team.marp-vscode`). O `.vscode/settings.json` na raiz do repositório já aponta o tema, desde que você abra **a raiz** no VS Code:

```json
{
  "markdown.marp.themes": ["./slides/themes/peteco.css"],
  "markdown.marp.enableHtml": true
}
```

O preview aparece ao lado do arquivo e o botão de exportar fica na paleta de comandos (`Ctrl+Shift+P` → *Marp: Export slide deck*).

### Três detalhes que economizam tempo

- **`--allow-local-files`** é obrigatório para exportar PDF/PPTX com as imagens da pasta `assets/`. Sem ele as imagens saem em branco.
- **`--no-stdin`** evita que o Marp fique parado esperando entrada quando roda dentro de script, CI ou terminal não interativo. Se o comando "travar" sem mensagem de erro, é isso.
- **O HTML mantém as imagens como caminho relativo.** Se o `.html` for para `build/`, a pasta `assets/` precisa ir junto (`cp -r assets build/assets`). O `make html` e o `npm run html` já fazem isso. PDF e PPTX embutem as imagens e não precisam disso.

> Para gerar PDF, o Marp usa um navegador Chromium. Se a máquina não tiver Chrome ou Edge instalado, rode uma vez `npx @marp-team/marp-cli@latest --version` e siga a instrução, ou aponte para um binário existente com a variável `CHROME_PATH`.

---

## Imagens

As fotos e prints (`foto-devkit.png`, `pinout.png`, `boards-manager.png`, `particoes.png`, `consumo-multimetro.png`) são material real. O `pagina-web-04.png` é a página do `exemplos/04b-servidor-web` renderizada no Chrome com dados de exemplo.

O `ph-pinout.svg` é o único placeholder que sobrou: está lá só para o `00-template.md` mostrar como se usa uma imagem.

Os diagramas de ligação das oficinas 2, 3 e 4 (`circuito-oled-i2c.svg`, `circuito-servo.svg`, `circuito-pratica-02.svg`, `circuito-pratica-03.svg` e `circuito-pratica-04.svg`) são **gerados** com [Schemdraw](https://schemdraw.readthedocs.io/). Para mudar um pino ou um rótulo, edite o script da oficina em `scripts/` e rode de novo. As peças comuns (cores do tema, bloco da ESP32, fios) ficam em `scripts/circuitos_comum.py`.

```bash
python3 -m venv .venv && .venv/bin/pip install schemdraw
.venv/bin/python scripts/circuitos_oficina2.py
.venv/bin/python scripts/circuitos_oficina3.py
.venv/bin/python scripts/circuitos_oficina4.py
```

Os dois esquemas da Oficina 1 (`circuito-ldr.svg` e `circuito-botao.svg`) foram desenhados à mão e são editáveis em qualquer editor de SVG (Inkscape, Figma) ou direto no texto.

O logo (`logo-peteco.png`) foi extraído do material do próprio grupo. Se existir uma versão vetorial, troque por ela.

---

## Criando a próxima oficina

```bash
cp 00-template.md 05-espnow-ble.md
```

O `00-template.md` traz um exemplo comentado de cada tipo de slide disponível: capa, divisória, código, tabela, duas colunas, armadilha, slide escuro, slide denso e encerramento. Ele **não entra** nos alvos `pdf` e `pptx` do Makefile, então pode ficar no repositório sem virar apresentação.

### Classes do tema

Aplique com `<!-- _class: nome -->` na primeira linha do slide (o `_` limita o efeito àquele slide; sem ele, vale do slide em diante). Dá para combinar: `<!-- _class: dark tight -->`.

| Classe | Para quê |
|---|---|
| `lead` | Capa. Fundo preto, faixa amarela, sem paginação |
| `section` | Divisória entre blocos da aula. Fundo amarelo |
| `dark` | Fundo escuro, para destaque ou fechamento de bloco |
| `alert` | Armadilha ou aviso. Fundo creme com barra amarela |
| `center` | Conteúdo centralizado na vertical e na horizontal |
| `tight` | Reduz a fonte quando o slide tem conteúdo demais |
| `end` | Slide final de agradecimento |
| `logo` | Marca-d'água do logo no canto inferior direito |

Duas colunas não são uma classe de slide, e sim um `div`:

```html
<div class="cols"><div>

conteúdo da esquerda

</div><div>

conteúdo da direita

</div></div>
```

As linhas em branco dentro das `div` são obrigatórias — sem elas o Markdown de dentro não é interpretado. Variantes: `cols` (1:1), `cols-1-2` e `cols-2-1`.

### Notas do apresentador

Comentários HTML que **não** começam com uma diretiva do Marp viram notas do apresentador:

```html
<!--
Lembrar de perguntar quantos já usaram Arduino.
Este bloco costuma levar 15 minutos.
-->
```

Elas aparecem no modo apresentador (tecla `P` no HTML), na exportação com `--pdf-notes` e nas notas do PPTX.

### Cores do tema

Estão todas em variáveis CSS no topo de `themes/peteco.css`:

```css
--peteco-amarelo: #f5c400;
--peteco-preto:   #14161a;
--peteco-cinza:   #5b6270;
```

Mudar a identidade visual das oficinas é mudar essas linhas.

---

## Convenções sugeridas

- **Um arquivo por oficina**, numerado: `01-`, `02-`, … O Makefile pega todos automaticamente
- **Uma ideia por slide.** Se precisou de `tight`, provavelmente o slide deveria ser dois
- **Código com no máximo ~16 linhas** por slide. O resto vai para `exemplos/`
- **Divisória `section` antes de cada bloco** — dá ritmo à aula e ajuda quem chegou atrasado
- **Sempre uma caixa `>` com a regra prática** no slide de armadilha: é o que a turma anota

---

## Conferindo o tamanho dos slides

O Marp não avisa quando o conteúdo passa da área útil do slide: ele simplesmente corta. O script `check_overflow.py` gera o HTML com o próprio Marp, abre no Chrome headless e **mede** a altura real do conteúdo de cada slide, com as fontes, imagens e o tema de verdade:

```bash
python3 check_overflow.py 0[1-9]-*.md
```

Saída típica (só aparecem os slides que merecem atenção):

```
  [no limite] slide 10     99%  Servo: PWM virando ângulo
  [ESTOURA  ] slide 21    104%  Ligando o display na bancada
  [CORTADO  ] slide  9    115%  Placas, não chips
```

- **no limite**: ocupa mais de 90% da área útil, mas cabe
- **ESTOURA**: passa da área útil e invade a faixa do rodapé
- **CORTADO**: passa da borda do slide; o que sobrou não aparece no PDF

A área útil é o slide sem os paddings do tema (onde ficam cabeçalho e rodapé). A porcentagem é a altura ocupada pelo conteúdo sobre essa área. O script sai com código 1 se algum slide estourar, então dá para usar em CI.

Usa o `marp` instalado no projeto ou com `npm -g`; sem nenhum dos dois, cai para o `npx`, que precisa de Node.js e de Chrome, Chromium ou Edge, que já são necessários para gerar o PDF. Se o navegador não for encontrado, aponte com `CHROME_PATH`. Imagens com margem branca podem dar **CORTADO** sem nada visível sumir: a caixa da imagem passou da borda, mas o que foi cortado é a margem.
