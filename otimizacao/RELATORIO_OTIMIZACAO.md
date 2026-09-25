# Copa APA: do código original ao ótimo provado

25/09/2026 · Deivison da Silva Costa

## 1 Resumo

As três instâncias da Copa APA foram resolvidas até o ótimo, e o ótimo foi provado matematicamente. Um ILS em C++ puro, sem nenhuma biblioteca externa, encontra os três ótimos partindo de uma solução gulosa, com tempo mediano de 21 s na n1000, 19 s na n700 e 49 s na n500 (seção 5).

| Instância | Entrega original na Copa | Melhor do ranking final | Ótimo provado | Tempo mediano até o ótimo (C++ puro, 1 thread) |
| --- | --- | --- | --- | --- |
| n500m10E | 18528 | 18448 | **18435** | 49 s (6 de 6 sementes chegam em 300 s) |
| n700m12E | 13541 | 13488 | **13090** | 19 s |
| n1000m15E | 5461 | 5118 | **5095** | 21 s |

A diferença para o código original não está em uma ideia única, mas em três mudanças: a vizinhança 2-opt\* (trocar as caudas de duas pistas), a avaliação de movimentos em tempo quase constante e a perturbação por ruína e recriação de trechos de tempo. A prova de otimalidade usou o solver HiGHS, mas só para provar; para encontrar os ótimos ele não é necessário.

## 2 O código original

O projeto entregue na Copa (branch `Deivison`, commit `287d2a0`, maio de 2025) tinha cerca de 1.400 linhas em C++, organizadas em quatro módulos: `Instance` (leitura e cálculo de custo), `GreedyAlgorithm` (construção), `VariableNeighborhoodDescent` (busca local) e `MetaHeuristics` (ILS e LNS). As entregas da Copa saíram do LNS.

### 2.1 Construção()

Os voos são ordenados por `r_j`, e cada um vai para a pista em que começa com a menor multa. A versão GRASP sorteia entre as pistas cuja multa fica até `(1 + α)` vezes a menor.

### 2.2 BuscaLocal()

VND com quatro vizinhanças, todas *best improvement*: troca de dois voos na mesma pista, troca entre pistas, reinserção na mesma pista e reinserção em outra pista. Cada movimento testado copiava a pista inteira e recalculava o custo do ponto alterado até o fim da pista:

```cpp
std::vector<int> newRunway = runway;          // cópia a cada movimento
std::swap(newRunway[i], newRunway[j]);
int costAfter = instance.calculatePartialRunwayCost(newRunway, impactPos, ...);  // até o fim
```

### 2.3 LNS (o que gerou as entregas)

```cpp
for (int iter = 0; iter < maxIterations; ++iter) {
    auto s = sol;
    auto removed  = destroy(s, currentK);   // k voos sorteados em qualquer lugar
    auto repaired = repair(s, removed);     // inserção mais barata
    vnd.vnd(instance, repaired);
    int c = instance.calculateTotalCost(repaired);
    if (c < curCost) { sol = repaired; curCost = c; ... }   // só aceita melhora
    else if (++semMelhora >= 100) currentK += 5;             // até 125, depois reinício GRASP
}
```

O `repair` testava cada posição de cada pista copiando a pista e recalculando seu custo inteiro. Isso dá O(n) por posição e O(n²) por voo reinserido, e o programa ainda imprimia uma linha por iteração.

### 2.4 Limitações que impediam chegar ao ótimo

1. **Poucas iterações por segundo**: cópias e recálculos completos deixavam cada movimento em O(n).
2. **Destruição sem estrutura**: os `k` voos removidos eram sorteados no horário inteiro. Como os atrasos se concentram em poucos trechos de tempo, quase toda remoção caía em regiões sem atraso e não ajudava.
3. **Aceitação só de melhoras**: sem aceitar pioras controladas, a busca ficava presa no primeiro ótimo local.
4. **Faltava a vizinhança certa**: nenhuma vizinhança trocava as caudas de duas pistas, que é o movimento de que este problema mais precisa (seção 6).

## 3 O que mudou

O código passou por três versões. A **original** é a da Copa. O **PR #1**, feito em 25/09/2026 pelo Claude da web, corrigiu bugs e acelerou o VND e o LNS mantendo a mesma estrutura. O **solver atual** (`ilssp`, compilado como `ilssp-pure`, sem bibliotecas externas) foi reescrito do zero em cima das propriedades do problema. As três rodaram na mesma máquina (i7-13650HX), na n500, por 60 s:

| Aspecto | Original (Copa) | PR #1 | Atual (`ilssp-pure`) |
| --- | --- | --- | --- |
| Resultado na n500 em 60 s | 23677 | 20962 (LNS), 20837 (ILS) | **18435 (ótimo)** |
| Iterações por segundo | 13 | ~130 | ~2.400 a 3.500 |
| Custo de avaliar um movimento | O(n): copia a pista e recalcula até o fim | O(n) no pior caso, com poda e parada antecipada | O(1) na prática: janela alterada + parada por sincronização (24 a 36 milhões de avaliações/s) |
| Posições testadas | Todas, em todas as pistas | Todas, em todas as pistas | Granular: só as vizinhas de `r_j` em cada pista (busca binária) |
| Vizinhanças | Troca e reinserção, na pista e entre pistas | + or-opt de 2 e 3 voos, RVND | + **2-opt\*** (troca de caudas entre pistas), deslocamentos curtos na pista |
| Reanálise após um movimento | VND inteiro de novo | VND inteiro de novo | Só os pares (voo, pista) perto do trecho alterado (*don't look bits*) |
| Perturbação | Remove 40 a 125 voos sorteados no horário todo | Idem, com reinserção incremental | Ruína e recriação local: 1 a 4 trechos de 1 a 4 voos seguidos, perto de um instante-semente |
| Aceitação | Só melhora | Só melhora | Simulated annealing |
| Desfazer um movimento rejeitado | Copia a solução inteira | Copia a solução inteira | Diário (*undo journal*) só das pistas tocadas |

A diferença de velocidade, cerca de 200 vezes mais iterações por segundo que o original, vem de três pontos: não copiar nada, avaliar só o trecho que muda e parar de avaliar assim que a pista volta a coincidir com a original. A diferença de qualidade vem da perturbação local, do 2-opt\* e da aceitação por simulated annealing.

## 4 Como o código funciona hoje

O solver atual segue o *framework* ILS da apostila, com duas diferenças: a busca local só reexamina a região que a perturbação mexeu, e a aceitação é por simulated annealing.

### 4.1 Framework

```cpp
Solution ILS(double tempo)
{
    Solution s = Construcao();                  // guloso por r_j
    BuscaLocal(s, TODAS_AS_REGIOES);
    Solution best = s;
    while (tempoRestante()) {
        double T = T0 * pMedio * pow(Tf / T0, fracaoDoTempo());  // resfriamento geométrico
        long long antes = s.cost;
        auto regioes = Perturbacao(s);          // ruína e recriação local
        BuscaLocal(s, regioes);                 // só perto do que mudou
        long long d = s.cost - antes;
        if (d <= 0 || uniforme() < exp(-d / T))
            confirma(s);                        // aceita (mesmo piorando um pouco)
        else
            desfaz(s);                          // undo journal: só as pistas tocadas
        if (s.cost < best.cost) best = s;
    }
    return best;
}
```

### 4.2 Construção()

Igual em espírito à original: voos em ordem de `r_j`, cada um na pista onde começa com a menor multa. No empate, fica com a pista de menor tempo ocioso antes do voo, e pistas vazias são o último recurso. A solução gulosa é ruim (54819 na n700), mas a primeira busca local já a leva a ~27 mil em menos de 0,1 s.

### 4.3 Avaliação de um movimento em O(1)

Cada pista guarda, para cada posição, o início `S` do voo e o custo acumulado `pc`. Um movimento troca um trecho `[lo, hi)` da pista por outra lista de voos. O prefixo não muda (custo `pc[lo]`), a lista nova é escalonada e, no sufixo antigo, a avaliação **para assim que um voo volta a começar no mesmo horário de antes**. Dali em diante nada muda.

```cpp
long long tailDelta(const Runway& R, int q, int prev, int endPrev) {
    long long d = 0;
    for (; q < R.size(); ++q) {
        int f = R.seq[q];
        int s = startAfter(prev, endPrev, f);    // max(r_f, fim anterior + t[prev][f])
        if (s == R.S[q]) break;                  // sincronizou: o resto da pista é igual
        d += p[f] * (s - R.S[q]);
        prev = f; endPrev = s + c[f];
    }
    return d;
}
```

Como quase todos os voos começam exatamente em `r_j` (seção 6), a sincronização acontece em 1 ou 2 voos. Na prática cada avaliação é O(1), e o solver chega a 24 a 36 milhões de avaliações por segundo.

### 4.4 BuscaLocal()

Para cada voo `x` e cada pista de destino `B`, só se testam as posições de `B` em volta de `r_x`, achadas por busca binária nos horários `S`. Colocar um voo muito longe do seu `r_x` raramente compensa. Os movimentos são:

1. **Relocate**: tirar `x` da pista `A` e inserir em `B`.
2. **Swap**: trocar `x` com um voo de `B`.
3. **2-opt\***: cortar `A` antes ou depois de `x`, cortar `B` no ponto correspondente e trocar as caudas. `A` fica com `A[0..ca) + B[cb..]` e `B` com `B[0..cb) + A[ca..]`. O custo sai de duas chamadas de `tailDelta`.
4. **Or-opt**: mover blocos de 2 e 3 voos seguidos.
5. **Deslocamentos na pista**: mover `x` até 3 posições para frente ou para trás.

A estratégia é *best improvement* por voo. Cada par (voo, pista) tem um bit de "sujo" (*don't look bits*): depois de um movimento, só os voos com `r_j` a até 100 unidades do trecho alterado são reexaminados, e só contra as pistas que mudaram.

### 4.5 Perturbacao()

Ruína e recriação local, no estilo SISR:

1. Sortear um voo-semente: metade das vezes um voo atrasado, metade qualquer voo. Seu horário é o instante-semente `t`.
2. Escolher de 1 a 4 pistas, sempre incluindo a da semente.
3. Em cada uma, remover de 1 a 4 voos consecutivos em volta de `t`.
4. Ordenar os removidos: por `r_j` (50%), aleatoriamente (25%) ou por multa decrescente (25%).
5. Reinserir cada um na posição mais barata perto de `r_j` em qualquer pista, pulando cada candidata com 3% de chance (*blinks*), para não repetir sempre a mesma escolha.

A perturbação é pequena e concentrada num instante, exatamente onde os voos disputam pista, e devolve as regiões alteradas para a busca local reexaminar só elas.

### 4.6 Parâmetros

| Parâmetro | Valor | Papel |
| --- | --- | --- |
| T0, Tf | 1,0 e 0,05 × multa média | Temperatura inicial e final do SA |
| Pistas por ruína | 1 a 4 | Alcance da perturbação |
| Voos por trecho | 1 a 4 | Tamanho da perturbação |
| Semente atrasada | 50% | Foco nas regiões com atraso |
| Blink | 3% | Diversificação na reinserção |
| Janela de reinserção | ±2 posições em volta de `r_j` | Granularidade da perturbação |
| Margem de reativação | 100 unidades de tempo | Alcance dos *don't look bits* |
| Janelas adaptativas | ±10 / ±15 / trechos até 8 se ≥ 30% dos voos atrasam | Instâncias congestionadas (seção 8) |

## 5 Como os ótimos foram encontrados e quanto tempo leva

Com 1 thread, o tempo mediano até o ótimo é de 21 s na n1000, 19 s na n700 e 49 s na n500.

O solver mede o próprio tempo: a opção `--target C` encerra a execução assim que o custo chega a `C`, e a linha final informa `best at X s`, o instante em que a melhor solução apareceu. Essa medição não custa velocidade: o relógio só é lido quando surge uma nova melhor solução, e o ILS já consultava o relógio a cada 32 iterações. A velocidade continuou em ~3.500 iterações e 36 milhões de avaliações por segundo.

Tempo até o ótimo com 1 thread, partindo do guloso, 10 sementes por instância (10 execuções em paralelo, todas as saídas conferidas pelo `tools/check.py`):

| Instância | Limite de tempo | Chegaram ao ótimo | Mediana | Mais rápida | Mais lenta |
| --- | --- | --- | --- | --- | --- |
| n1000m15E (5095) | 30 s | 10 de 10 | 20,9 s | 12,2 s | 29,9 s |
| n700m12E (13090) | 60 s | 8 de 10 | 19,0 s | 9,3 s | 42,8 s |
| n500m10E (18435) | 120 s | 6 de 10 | 49,0 s | 23,0 s | 60,3 s |

Com mais tempo a taxa de sucesso sobe: na n500 com 300 s foram 6 de 6 sementes, e na n700 com 300 s, 11 de 12. Como as sementes são independentes, rodar 10 em paralelo (uma por núcleo) faz a primeira chegar ao ótimo em cerca de 23 s na n500, 9 s na n700 e 12 s na n1000.

Quando a execução não chega ao ótimo, costuma terminar a menos de 0,5% dele (18444 a 18466 na n500, 13154 na n700). O pior caso foi 13346 na n700 com 30 s, 2% acima.

### 5.1 O caminho até aqui

1. **Diagnóstico das instâncias.** Antes de mexer em algoritmo, medimos a estrutura dos dados: quase todo voo pode começar em `r_j`, e os atrasos se concentram em poucos trechos de tempo (seção 6).
2. **Cinco abordagens em paralelo.** Ruína e recriação com SA (SISR), ILS granular com 2-opt\* (GLS), algoritmo genético híbrido (HGS), decomposição em janelas de tempo resolvidas por MIP e ILS com *set partitioning* (ILS-SP). O GLS achou 18435 na n500. A decomposição por MIP achou 13090 e 5095.
3. **Convergência.** Três métodos independentes pararam nos mesmos valores, o que já sugeria que eram os ótimos. O ILS-SP juntou as melhores ideias e chegava a 18435 / 13107 / 5095 em ~20 s. O *set partitioning* (MIP no HiGHS) fechava o 13090 da n700 em ~30 s.
4. **Prova de otimalidade.** A relaxação por janelas de tempo com *reduced cost fixing* provou os três valores (seção 6.3).
5. **C++ puro.** Na n700, a solução de 13108 já tinha os horários do ótimo, com exceção de um voo (616, atrasado 2 unidades a mais). Faltava só reorganizar as cadeias de voos entre as pistas, e isso o ILS faz sozinho com um pouco mais de tempo. O `ilssp-pure` é o mesmo ILS compilado sem o HiGHS e chega aos três ótimos (tabela acima).

## 6 Por que funciona

O método funciona porque cada peça explora uma propriedade das instâncias: há folga de sobra entre os voos, os atrasos são raros e concentrados, e o problema real é decidir onde as cadeias de voos se juntam.

### 6.1 A estrutura das instâncias

Nas instâncias da Copa, `c` e `t` ficam entre 1 e 50 e `p` entre 1 e 100, com `t` assimétrica. Nas soluções ótimas:

| Instância | Voos atrasados | Maior atraso | Voos por pista | Cadeias mínimas sem atraso | Pistas |
| --- | --- | --- | --- | --- | --- |
| n500m10E | 78 (15,6%) | 211 | 44 a 55 | 15 | 10 |
| n700m12E | 70 (10,0%) | 123 | 53 a 63 | 18 | 12 |
| n1000m15E | 45 (4,5%) | 93 | 62 a 74 | 20 | 15 |

"Cadeias mínimas sem atraso" é o menor número de pistas que permitiria atender todos os voos exatamente em `r_j`. Calcula-se como uma cobertura mínima por caminhos, via emparelhamento bipartido. Como esse número passa de `m`, algumas cadeias precisam ser fundidas, e cada fusão gera atraso. O problema vira escolher **onde** fundir as cadeias para que o atraso ponderado seja mínimo.

### 6.2 Por que cada peça do solver funciona

1. **Avaliação O(1).** Como quase todo voo começa em `r_j` com folga, uma mudança na pista é absorvida em 1 ou 2 voos. A parada por sincronização aproveita exatamente isso.
2. **Vizinhança granular.** Um voo colocado longe de `r_j` ou fica ocioso ou empurra os vizinhos. Basta testar as posições em volta de `r_j`, o que reduz a vizinhança de O(n) para O(1) por pista.
3. **2-opt\*.** A solução é um conjunto de cadeias, e trocar o "final" de duas pistas é o jeito natural de mudar onde as cadeias se fundem. Relocate e swap mexem em um voo por vez e não conseguem fazer isso.
4. **Ruína local.** Os atrasos se concentram em poucos trechos de tempo (na n1000, cinco trechos concentram todo o custo). Destruir em volta de um voo atrasado ataca direto a região que importa, enquanto a destruição aleatória do código original caía quase sempre em trechos sem atraso.
5. **Simulated annealing.** Aceitar pioras pequenas no começo permite atravessar as reorganizações de cadeias que precisam de vários passos. É esse tipo de reorganização que separa o 13108 do 13090 na n700: os horários são os mesmos, exceto por um voo 2 unidades mais atrasado, mas 342 ligações entre voos mudam.

### 6.3 A prova de otimalidade

O custo de qualquer solução se divide exatamente entre janelas de tempo disjuntas `W = [a, b)`:

```math
C(S) = \sum_j p_j (S_j - r_j) = \sum_{W} \sum_j p_j \, \big| [r_j, S_j) \cap W \big|
```

Para cada janela, resolve-se uma **relaxação** que olha só os voos que começam nela: os voos com `r_j` na janela começam nela ou são empurrados para depois de `b` (custo `p_j(b - r_j)`); os voos anteriores podem ser puxados para dentro (custo `p_j(S_j - a)`) ou não (custo 0); e o primeiro voo de cada pista começa em `max(r_j, a)`. Qualquer solução real gasta na janela pelo menos o ótimo dessa relaxação.

Se, em toda janela, a relaxação provar que nenhuma escala custa menos que a contribuição `UB_W` da nossa solução, então nenhuma solução custa menos que a nossa: ela é ótima. A relaxação é um fluxo em rede indexado por tempo (nó = voo × horário de início) resolvido no HiGHS. O LP bastou em quase todas as janelas.

Nas duas janelas em que o LP ficou abaixo, entrou o **reduced cost fixing**. Com os duais `y` do LP, o limitante Lagrangiano `L(y)` vale para qualquer `y`, e cada variável `x_k` com custo reduzido `d_k` só pode valer 1 numa solução de custo pelo menos `L(y) + d_k`:

```math
L(y) + d_k > UB_W - 1 \;\Longrightarrow\; x_k = 0
```

Isso eliminou mais de 99% das variáveis (sobraram 22.533 de 4,9 milhões na n500 e 20.720 de 4,06 milhões na n700), e o MIP no modelo reduzido fechou cada janela em menos de 1 s, com gap 0%.

| Instância | Janelas (contribuição provada) | Soma |
| --- | --- | --- |
| n500m10E | [0, 1300) 6266 · [1300, 1800) 7498 · [1800, 3000) 4671 | **18435** |
| n700m12E | [0, 1330) 8240 · [1330, 2600) 3824 · [2600, 3700) 1026 | **13090** |
| n1000m15E | [210, 733) 840 · [805, 1423) 2085 · [1471, 1988) 760 · [2200, 3554) 1410 | **5095** |

Em cada instância, as janelas cobrem todo o atraso da solução, e fora delas o atraso é zero. A soma dos limitantes é igual ao custo da solução, o que prova a otimalidade.

## 7 Como reproduzir

Tudo fica na pasta `otimizacao/` do repositório. O solver em C++ puro só precisa de g++ com C++20:

```sh
cd otimizacao/solvers/ilssp
make ilssp-pure
./ilssp-pure ../../instances/n700m12E.txt --time 60 --seed 1 --no-sp --target 13090 --out saida.txt
python3 ../../tools/check.py ../../instances/n700m12E.txt saida.txt
```

| Opção | Efeito |
| --- | --- |
| `--time T` | Limite de tempo em segundos (o resfriamento do SA se estende por ele) |
| `--seed S` | Semente |
| `--threads K` | K trajetórias independentes |
| `--target C` | Para ao atingir custo C e informa o tempo |
| `--no-sp` | Sem set partitioning (obrigatório no `ilssp-pure`) |
| `--init arq` | Parte de uma solução existente |
| `--no-adapt` | Desliga a escolha automática das janelas (seção 8) |

A prova de otimalidade (`winlb`) e o ILS com set partitioning (`ilssp`) usam o HiGHS, que vem com o pacote Python `highspy`:

```sh
python3 -m venv venv && venv/bin/pip install highspy numpy scipy
cd otimizacao/solvers/ilssp
make ilssp winlb HIGHSDIR=$(realpath ../../../venv/lib/python3.*/site-packages/highspy)
./winlb ../../instances/n700m12E.txt ../../solucoes_otimas/n700m12E.txt --window 0:1330 --time 4000 --rcfix
```

Cada janela da prova usa de 1 a 4 GB de RAM e leva de 5 a 20 minutos. Com 15 GB de RAM, rode uma janela por vez.

## 8 Instâncias da disciplina

O solver também foi testado nas 20 instâncias da disciplina (`Instances/`). Elas são muito mais congestionadas que as da Copa: depois da primeira busca local, 38% a 90% dos voos estão atrasados, contra 6% a 19% na Copa. Nessas condições, a busca granular com janelas estreitas em volta de `r_j` perdia posições boas. No n5m50A, por exemplo, só 2 de 20 sementes chegavam ao melhor valor conhecido.

Por isso o solver escolhe as janelas sozinho. Antes da busca, uma busca local a partir do guloso mede a fração de voos atrasados. A partir de 30%, as janelas são alargadas: posições ±10 em volta de `r_j` na busca local, ±15 na reinserção e trechos de até 8 voos na ruína. As instâncias da Copa ficam com as janelas estreitas, sem mudança de comportamento. Passar `--pos-window`, `--insert-window` ou `--max-string` na linha de comando, ou usar `--no-adapt`, desliga a escolha automática.

Resultado com 5 sementes de 5 s cada. A referência é o ótimo, quando conhecido, ou o melhor valor do ILS original:

| Instâncias | Referência | Solver novo |
| --- | --- | --- |
| n3m10 A a E | 7483, 1277, 2088, 322, 3343 (ótimos) | Todas iguais, 5 de 5, em menos de 0,01 s |
| n3m20 A a E | 31357, 16719, 6462, 4357, 3798 | Todas iguais, 5 de 5, em menos de 0,01 s |
| n3m40A | 45474 | 5 de 5 (antes da adaptação: 1 de 3) |
| n3m40B, D, E | 28971, 12963, 17838 | 5 de 5 |
| n3m40C | 43665 | 2 de 5; com 20 sementes, 3 de 20 contra 5 de 20 da janela estreita (média igual: 43807 x 43808) |
| n5m50A | 54401 | 3 de 5; com 20 sementes, **17 de 20** contra 2 de 20 da janela estreita |
| n5m50B a E | 23739, 14510, 15240, 6327 | 5 de 5 |

O solver novo empata com a referência em todas as 20 instâncias, mas não melhora nenhuma, o que sugere que esses valores também sejam ótimos (não provado). Nas instâncias da Copa, a validação depois da mudança manteve o comportamento: 5 de 5 na n500 em 120 s, e 4 de 5 na n700 e na n1000.
