# APA-Project
Projeto da disciplina de Análise e Projeto de Algoritmos

Allejandro Sousa dos Santos
Deivison da Silva Costa
Giancarlo Silveira Cavalcante

## Problema

Escalonamento de pousos e decolagens em `m` pistas: cada voo `i` tem horário de
liberação `r_i`, duração `c_i` e multa por unidade de atraso `p_i`; entre voos
consecutivos `i → j` na mesma pista é obrigatório o intervalo `t_ij`. O objetivo é
minimizar `Σ p_i · (início_i − r_i)`.

## Compilação

```sh
make        # gera ./apa_project
make test   # confere o exemplo do enunciado (custo 2800)
```

## Uso

```sh
./apa_project Instances/n3m10A.txt                 # ILS (padrão), grava em results/
./apa_project Instances/n3m10A.txt --algo vnd      # guloso + VND
./apa_project copa_apa/n500m10E.txt --algo lns --time 300 -v
./apa_project Instances/n3m10A.txt --check Instances/n3m10A_7483.txt
./apa_project --bench --runs 10                    # tabela de resultados (results/tabela.md)
```

Opções: `--algo greedy|grasp|vnd|rvnd|ils|lns`, `--out DIR`, `--seed S`, `--time T`,
`--threads T`, `--restarts N`, `--ils-iter N`, `--lns-iter N`, `--init ARQ`, `-v`.

## Algoritmos

- **Guloso** (`GreedyAlgorithm`): voos em ordem de liberação, cada um na pista com a
  menor multa; a versão GRASP sorteia entre as pistas da lista restrita de candidatos.
- **Vizinhanças** (`VariableNeighborhoodDescent`), todas com *best improvement*:
  swap na pista, swap entre pistas, or-opt de blocos de 1, 2 e 3 voos na pista e
  entre pistas.
- **VND / RVND**: VND clássico (ordem fixa) e a versão aleatória usada no ILS.
- **ILS**: reinícios GRASP distribuídos entre threads (OpenMP), perturbação com
  *double bridge* na pista e troca/realocação de segmentos entre pistas.
- **LNS**: destrói `k` voos aleatórios, reinsere na posição mais barata e aplica RVND.

### Avaliação eficiente dos movimentos

Nenhum movimento copia vetores. Para cada pista são mantidos o horário de início e
o custo acumulado de cada posição, de modo que o prefixo inalterado custa O(1).
O sufixo é avaliado até que:

1. o custo parcial ultrapasse o da melhor melhora já encontrada (poda), ou
2. um voo volte a ter o mesmo antecessor e o mesmo horário de início que na solução
   original — daí em diante nada muda e o restante do custo vem das estruturas
   auxiliares.
