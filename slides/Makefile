# Oficinas de ESP32 — PETECO
#
#   make            → HTML de todas as oficinas (em build/)
#   make pdf        → PDF de todas (com notas do apresentador)
#   make pptx       → PPTX de todas
#   make watch      → recompila e recarrega o navegador enquanto você edita
#   make check      → mede quais slides estouram a altura
#   make clean

MARP  ?= npx --yes @marp-team/marp-cli@latest
FLAGS := --no-stdin --theme-set themes/ --allow-local-files
DECKS := $(filter-out 00-template.md,$(wildcard [0-9][0-9]-*.md))
BUILD := build

.PHONY: all html pdf pptx watch server check clean

all: html

# O HTML mantém as imagens como caminho relativo, então assets/ vai junto.
html: | $(BUILD)
	cp -r assets $(BUILD)/assets
	@for d in $(DECKS); do \
		echo ">> $$d"; \
		$(MARP) $(FLAGS) "$$d" -o "$(BUILD)/$${d%.md}.html"; \
	done

# PDF e PPTX já embutem as imagens: não precisam da pasta assets ao lado.
pdf: | $(BUILD)
	@for d in $(DECKS); do \
		echo ">> $$d"; \
		$(MARP) $(FLAGS) --pdf --pdf-notes --pdf-outlines "$$d" -o "$(BUILD)/$${d%.md}.pdf"; \
	done

pptx: | $(BUILD)
	@for d in $(DECKS); do \
		echo ">> $$d"; \
		$(MARP) $(FLAGS) --pptx "$$d" -o "$(BUILD)/$${d%.md}.pptx"; \
	done

watch:
	$(MARP) $(FLAGS) --watch --preview $(firstword $(DECKS))

server:
	$(MARP) $(FLAGS) --server .

check:
	@for d in $(DECKS); do echo ">> $$d"; python3 check_overflow.py "$$d" || true; done

$(BUILD):
	mkdir -p $(BUILD)

clean:
	rm -rf $(BUILD)
