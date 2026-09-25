# ilssp — ILS with Set-Partitioning recombination

Solver for P | r_j, s_ij | sum p_j (S_j - r_j) (Copa APA runway scheduling).

## Build / test

    make            # g++ -O3 -march=native, links the libhighs.so shipped in the highspy venv
    make test       # tests/selftest.sh: verifies every applied move delta and all caches
                    # inside the solver (--selftest) and checks outputs with tools/check.py

## Run

    ./ilssp <instance.txt> --time 300 --seed 1 --threads 1 [--init sol.txt] [--out file|dir]
            [--pool-dir ../../pool/<inst>]   # ingest other solutions' runways as SP columns
            [--no-sp] [--round 30] [--sp-time 10] [--sp-max-cols 5000]

The best solution is (re)written to `--out` after every round and at the end
(`<dir>/<inst>_ilssp.txt` when `--out` is a directory).

## Algorithm

* **Evaluation**: runways keep start times and prefix costs; a move is evaluated by
  recomputing only the disturbed window and stopping as soon as a flight of the old
  suffix gets its old start time back (sync-stop). Typically O(1).
* **Local search** (granular): for a flight x and a target runway B, the candidate
  positions are the ones around x's release time in B (binary search on start times).
  Moves: relocate, swap, 2-opt* (tail exchange, cut before/after x), or-opt (2-3 flights),
  intra-runway shifts by up to 3 positions. Per-(flight, runway) dirty bits: after a
  move only flights released near the changed time region are re-examined, and only
  against the runways that changed.
* **Perturbation**: ruin & recreate (SISR-like): strings of 1-4 consecutive flights are
  removed from 1-4 runways around a seed time (seed = a delayed flight half of the time),
  then reinserted at the cheapest position with 3% blinks. Undo journal for rejection.
* **Acceptance**: simulated annealing, T from 1.0 to 0.05 times the mean penalty over
  each round; each round restarts from the global best.
* **SP**: accepted local optima within 0.2% of the best feed a hashed column pool.
  After each round, a set-partitioning MIP (cover every flight once, at most m columns,
  warm-started with the incumbent) is solved with HiGHS through its C API. Presolve is
  off because it is slow on these long columns and ignores the time limit.
* `--threads k` runs k independent trajectories per round and merges their pools.

## Files

`src/eval.hpp` (sync-stop evaluator), `src/localsearch.*`, `src/ruin.*`, `src/ils.*`,
`src/colpool.*`, `src/spsolver.*` + `src/highs_min.h` (HiGHS C API subset),
`src/driver.*` (rounds, threads, SP), `src/editor.*` (move application + undo journal),
`publish.sh` (validate + atomic publish to the shared pool).
