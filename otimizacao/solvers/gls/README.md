# gls — granular local search + ILS for P | r_j, s_ij | Σ p_j (S_j − r_j)

Build: `make` (g++ -O3 -march=native, C++17). Self-test: `make test`.

```
./gls <instance> [--time s] [--seed k] [--threads k] [--init sol] [--out file|dir]
```
The best solution is written to `--out` every few seconds while it improves and at the end
(if `--out` is a directory the file is `<dir>/<instance>_gls.txt`). The final line prints
`FINAL cost=... reference=...` (the incremental cost re-checked with a plain O(n) evaluation).

## Algorithm
* **State / evaluation** (`state.*`): per runway the sequence, start times and prefix costs.
  Every candidate runway is `X[0..a] + E + Y[b..]`; its cost is `cum_X[a+1] + cost(E)` plus a
  suffix recomputation that stops as soon as a flight gets its current start time again
  (sync-stop), so evaluating a move is O(1) in practice (~10–15 ns). Partial costs are
  compared with a limit for early rejection.
* **Granular local search** (`localsearch.*`): candidate lists of the K=30 best predecessors /
  successors of each flight (score = delay cost + idle time). For flight f and candidate g the
  moves creating arc g→f (or f→h) are: relocation of strings of 1..3 flights, cross-exchange of
  strings (1..3 × 1..3), 2-opt* tail exchange, intra-runway relocation/swap, and moves to the
  start of other runways. Best-improvement per flight, driven by a queue of dirty flights
  (don't-look bits: a flight is re-examined only if its predecessor, successor or start changed).
* **Assignment neighborhoods** (`assign.*`): cut all runways at time T (or at T1 and T2) and
  re-match prefixes with tails (or frames with middle segments) optimally with the Hungarian
  algorithm — an exact cyclic generalization of 2-opt* / cross-exchange over m runways.
  Applied around the perturbed time after each local search.
* **ILS** (`perturb.*`, `ils.*`): time-localized ruin & recreate (a window of flights by release
  time, or strings of a few runways around a time; the seed is preferably a delayed flight),
  greedy best insertion with blinks and random orders, then local search on dirty flights only,
  then simulated-annealing acceptance (T decays geometrically from `--T0` 300 to `--Tf` 5 within
  each annealing cycle; `--cycle 100` restarts the schedule from the chain's best solution every
  ~100 s). Undo is a lazy per-runway backup (only runways touched in the iteration are copied).
* **Threads**: `--threads k` runs k independent chains; a chain adopts the global best (checked
  every `--sync` seconds) only after it stagnated for 2 sync periods.
* Diagnostics: `--check` re-verifies the incremental cost against a plain evaluation after every
  iteration; `--sweep` applies the assignment neighborhoods exhaustively; `--donor sol` runs the
  window-import crossover (`recombine.*`) of `--init` with donor solutions.

## Results (validated with tools/check.py)
| instance | cold 300 s, seed 1 (time of best) | warm 600 s |
|---|---|---|
| n500m10E | 18435 (53.9 s) | see report |
| n700m12E | 13090 (43.3 s) | see report |
| n1000m15E | 5095 (59.4 s) | see report |

Speed (3 runs in parallel on a shared machine): 1600–2000 ILS iterations/s, 50–60 M move
evaluations/s (up to ~3600 it/s and ~100 M evals/s on an idle core); ~38 flights examined per
iteration.

## Files
`src/common.hpp` RNG/timer · `src/instance.*` I/O and reference cost · `src/state.*` evaluator ·
`src/neighbors.*` candidate lists · `src/localsearch.*` granular LS · `src/assign.*` Hungarian
neighborhoods · `src/perturb.*` ruin & recreate · `src/recombine.*` window-import crossover ·
`src/ils.*` driver · `src/main.cpp` CLI.
`publish.sh <inst> <sol>` validates with the independent checker and publishes to the pool if
strictly better. `batch.sh <jobfile>` runs tuning jobs, 3 at a time.
