# window — time-window matheuristic + granular ILS for P | r_j, s_ij | Σ p_j (S_j − r_j)

## Build

```
make highs   # once: clones HiGHS v1.15.1 into third_party/ and builds a static lib (3 jobs)
make         # builds ./window (C++20, -O3 -march=native)
make test    # self-tests: incremental evaluator vs. from-scratch cost, LS deltas vs. real cost
```

## Run

```
./window <instance> [--time s] [--seed k] [--threads k] [--init sol.txt] [--out file|dir]
         [--pool dir --checker check.py]   # publish improvements (validated by check.py, atomic mv)
         [--mode hybrid|ils|window|hybrid0|onewin|xtest|selftest]
         [--epochs k] [--wsize flights] [--wtl mipSeconds] [--wfrac share] [--focus p]
```

Default mode `hybrid`: epochs of annealed ILS (fresh greedy start per epoch, the first epoch
from `--init` if given), a window-MIP sweep over each epoch result, and exact time-cut
crossover of every epoch result (and of every improved child) against an elite archive.
The best solution is always written to `--out` (directory → `<inst>_window_best.txt`).
`--threads` is accepted; the solver is single-threaded (HiGHS is run with 1 thread).

## Components

* `solution.*` — runways as sequences with start times and prefix costs; `evalConcat`
  evaluates "prefix of runway P + inserted flights + suffix of runway Q" with a sync-stop (as soon
  as a suffix flight starts at its old time, the rest of the cost is reused) → O(1) moves in practice.
* `ls.*` — granular LS (40 release-time neighbours): relocate, swap, 2-opt* (both arc directions),
  or-opt of 2–3 flights, segment/flight swap, intra-runway relocate/swap; don't-look queue.
* `search.*` — greedy construction, ruin & recreate (4–14 time-neighbours, best reinsertion),
  ILS with simulated-annealing acceptance on local optima.
* `window.*` — the time-window subproblem: flights starting in [T0,T1) on the chosen runways are
  freed, prefixes are fixed, and the post-window suffix chains may be re-attached to any runway.
  Exact model: arc-time-indexed 0/1 flow over states (flight, delay) with the real cost (suffix
  chain cost evaluated exactly as a function of its head start). Before building it, an m+|F|
  assignment relaxation (Hungarian) gives a lower bound; if it equals the incumbent the window is
  proven optimal without a MIP, otherwise its duals are used for reduced-cost filtering of states
  and arcs (forward/backward labels). Solved with HiGHS (static, 1 thread), incumbent as start.
* `crossover.*` — time-cut crossover: prefix of A before a clean cut T + suffix chains of B,
  re-attached by an exact m×m assignment on the true junction costs; all clean cuts are scanned.
* `memetic.*` — epoch / archive driver; `matheur.*` — window sweeps; `publish.*` — output + pool.
