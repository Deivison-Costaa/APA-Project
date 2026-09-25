# hgs — Hybrid Genetic Search for the Copa APA runway problem

Problem: `P | r_j, s_ij | sum p_j (S_j - r_j)` (m identical runways, releases, asymmetric
sequence-dependent gaps, weighted delay). HGS in the style of Vidal (population of local
optima, biased fitness = cost rank + diversity rank, restarts), adapted to parallel runways.

## Build / run

```
make                 # g++ -O3 -march=native -std=c++20
make test            # self-tests + short runs validated by tools/check.py
./hgs <instance> --time 300 --seed 1 --threads 1 [--init sol.txt] --out <file|dir>
```

`--out` may be a directory (`<dir>/<inst>_hgs_s<seed>.txt`). The best solution is written
(atomically) whenever it improves (at most every 2 s) and at the end; stdout shows progress
and a `FINAL <cost> <file>` line. `--threads k` runs k independent islands (seeds differ)
that share the global best at their elite restarts.

Tuning flags (defaults in `src/params.hpp`): `--xover mix|tw|ruin|rx|none`, `--mu 25`,
`--lambda 40`, `--itNoImp 4000`, `--nbCorr 20 --nbTime 10`, `--winMin 0.1 --winMax 0.6`,
`--pTw 0.7`, `--ruinMin 0.01 --ruinMax 0.08`, `--dist starts|arcs|mix`, `--keepBest 0|1`,
`--eliteEvery 3`, `--ejection 0|1`, `--focused 0|1`. Diagnostics: `--selftest`,
`--polish --init sol` (descent + multi-runway tail reassignment on a given solution).

## Algorithm

* **Local search (education)** — `src/local_search.*`. Every move is at most three
  *edits*; an edit rewrites a runway as `prefix + L + tail(of some runway, from position q)`.
  The delta is `sum p_x (S'_x - S_x)` over `L` and the tail, and the tail scan stops at the
  first flight whose start time is unchanged (sync-stop), so a move costs O(1) in practice.
  Moves per neighbour pair (u, v): relocate u after/before v, swap, both 2-opt* tail
  exchanges, swap u with (v,v2), relocate (u,u2) and (u,u2,u3), swap (u,u2) with v and
  with (v,v2); intra-runway relocate/swap/or-opt; moves to empty runways. Granular
  neighbour lists: 20 best "fit" flights (predecessor/successor slack) + 10 closest
  releases. First improvement; removal deltas are cached per flight.
* **Focused education**: a child is compared with its parents; only flights whose
  (pred, succ, start) match neither parent are dirty, and a pair is re-examined only if a
  flight in the window pred/self/succ/succ2 of u or v changed since u was last examined.
  This gave ~6x more generations per second than re-scanning the whole child.
* **Crossover (time-window, "tw")**: child = parent A outside a random window
  [T1, T2) (10–60 % of the horizon), parent B inside. Runway pieces are joined by two
  m x m Hungarian assignments (A-prefix -> B-middle, then -> A-suffix) whose costs are the
  exact sync-stop junction deltas; flights lost at the borders are reinserted cheapest-first.
* **Runway crossover ("rx")**, SREX-like: k runways of A + the m-k runways of B that lose
  the fewest flights; missing flights reinserted. Measured and discarded (weak).
* **Ruin & recreate mutation**: remove the flights of a time window (1–8 % of the horizon,
  centred on a delayed flight with prob. 0.7) from a random half of the runways, reinsert.
  Default operator mix: 70 % tw crossover, 30 % ruin.
* **Population**: mu = 25, lambda = 40, 4 elites, 5 closest. The diversity distance is the
  fraction of flights with a different start time: good solutions differ in hundreds of
  zero-cost arcs, so the classic broken-pairs distance kept "clones" (same start vector,
  same cost) and the search stalled (18466 vs 18435 in 60 s on n500).
* **Restarts**: after 4000 generations without improving the run's best, restart from
  random greedy individuals; the run's best goes to an archive, and every third restart
  starts from the archive of run bests (elite recombination).

See the final report of the workflow for the measured results.
