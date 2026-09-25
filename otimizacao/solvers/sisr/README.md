# sisr: string-removal ruin & recreate + 2-opt* in simulated annealing

Solver for `P | r_j, s_ij | sum p_j (S_j - r_j)` (Copa APA runway scheduling).

## Build and run

```
make                # ./sisr (-O3 -march=native)
make test           # debug build + self-test (incremental cost checked every move + check.py)
./sisr <instance> --time 300 --seed 1 [--threads k] [--init sol.txt] [--out file|dir]
```

`--out DIR` writes `DIR/<inst>_<cost>_sisr.txt`; `--out FILE` writes FILE. The best
solution is always written at the end (atomic tmp + rename). Progress is printed every 5 s.

## Algorithm

State: `m` runways, each a vector of flights with start times `S` and finish times `F`.
Every modification is re-timed with the sync stop: the recomputation of a runway suffix
stops as soon as a start time equals the old one, so moves cost O(1) in practice.

Each SA iteration is one of two moves:

1. **SISR ruin & recreate** (Christiaens & Vanden Berghe 2020, adapted):
   * seed flight (biased 50% towards delayed flights); neighbours are flights close in release time;
   * ruin: 1..3 strings (length <= 10, 50% "split string" that keeps a middle block) on the
     runways of the seed's time neighbours, or (20%) a time slice `[r_s - w, r_s + w]` cut out of
     2..m runways;
   * 30%: the tails of two ruined runways are exchanged at the cut points (2-opt* inside the ruin);
   * recreate: order random / by release / by penalty / by release desc; each flight goes to its
     cheapest position over all runways; candidate positions start at the first flight with
     `S >= r_j - 60` and stop when the flight's own delay alone exceeds the best delta;
     insertion delta is exact with sync stop and bound pruning; blink rate 1%.
2. **Best 2-opt\*** (50%): cut the seed's runway before/after the seed and try every other runway
   cut at the time-aligned position (+-1); exact delta of both re-rooted tails with sync stop.

Acceptance: `new < cur - T ln U`. Schedule:
* 10% of the time: global SA, T from 100 to 10 exponentially in time;
* rest: **windowed SA** restarts from the best solution. A window is a cluster of delayed
  flights (delayed flights closer than 60 in start time, chosen with probability ~ its cost,
  plus a random 20..120 margin) or (30%) a random time window; seeds are drawn only from flights
  released in the window; 100k iterations with T from 100 to 5. The cost is concentrated in a few
  independent "hot spots", so optimising them one at a time and keeping the best is much more
  reliable than one global anneal (the global anneal freezes every hot spot at the same time).

`--threads k` runs k independent chains (different seeds) sharing the global best.

## Files

* `src/runway.hpp` incremental runway timing, insertion / tail evaluation
* `src/sisr.cpp` ruin, recreate, 2-opt*, SA and window schedule
* `src/solution.cpp` solution IO and independent evaluation; `src/instance.cpp` reader
* `tests/selftest.sh` self-test; `tests/batch.sh`, `tests/seeds.sh` experiment runners;
  `tests/publish.sh` validates (check.py) and publishes atomically to the pool;
  `tests/hotspots.py` prints the delay clusters of a solution.

## Main parameters (defaults tuned on the three Copa instances)

`--T0 100 --Tf 10 --global 0.1 --wT0 100 --wTf 5 --witers 100000 --opt 0.5 --tail 0.3
--slice 0.2 --slicew 100 --bias 0.5 --hot 0.7 --cgap 60 --margin 20,120 --cbar 10 --lmax 10 --blink 0.01`
