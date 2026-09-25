# Otimização pós-Copa: soluções ótimas provadas

As três instâncias da Copa APA 2025.1 foram resolvidas e **provadas ótimas** em 25/09/2026.
O relatório completo, com a comparação com o código original, está em
[RELATORIO_OTIMIZACAO.md](RELATORIO_OTIMIZACAO.md).

| Instância | Ótimo | Melhor do ranking final | Entrega da equipe na Copa |
|---|---|---|---|
| n500m10E | **18435** | 18448 | 18528 |
| n700m12E | **13090** | 13488 | 13541 |
| n1000m15E | **5095** | 5118 | 5461 |

## Estrutura

- `solucoes_otimas/`: as três soluções ótimas (formato da Copa: custo, depois uma linha por pista).
- `instances/`: as instâncias da Copa (as mesmas de `copa_apa/`).
- `tools/check.py`: verificador independente (recalcula o custo e checa voos e pistas).
- `tools/lb.py`: limitante de designação e número mínimo de cadeias sem atraso.
- `pool/`: melhores soluções encontradas por cada solver.
- `solvers/`:
  - `ilssp/`: **o solver principal**. ILS com ruína e recriação, busca local granular com 2-opt\* e
    simulated annealing. `make ilssp-pure` compila a versão em C++ puro, sem dependências.
    - `lb/`: `winlb`, a prova de otimalidade por janelas de tempo (usa o HiGHS).
    - `lbruns/`: logs de todas as provas.
    - `ttt/`, `bench/`, `pure_runs/`, `cpp700/`: logs e soluções das medições de tempo.
  - `gls/`: ILS granular com 2-opt\* (achou 18435 primeiro). C++ puro.
  - `sisr/`: ruína e recriação com SA. C++ puro.
  - `hgs/`: algoritmo genético híbrido. C++ puro.
  - `window/`: decomposição em janelas de tempo com MIP (HiGHS estático; `make highs` baixa e compila).

## Uso rápido

```sh
cd solvers/ilssp
make ilssp-pure
./ilssp-pure ../../instances/n700m12E.txt --time 60 --seed 1 --no-sp --target 13090 --out saida.txt
python3 ../../tools/check.py ../../instances/n700m12E.txt saida.txt
```

A linha final mostra o custo, o instante em que a melhor solução apareceu (`best at X s`) e a velocidade
(iterações e avaliações por segundo). Os comandos da prova de otimalidade estão na seção 7 do relatório.
