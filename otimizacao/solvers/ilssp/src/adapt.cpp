#include "adapt.hpp"

#include "ils.hpp"

namespace {

constexpr double kWideThreshold = 0.30;  // delayed fraction above which windows are widened
constexpr int kWidePosWindow = 10;       // LS positions tried around r_j (default 1)
constexpr int kWideInsertWindow = 15;    // ruin reinsertion positions around r_j (default 2)
constexpr int kWideMaxString = 8;        // flights removed per runway by a ruin (default 4)

}  // namespace

AdaptResult adaptWindows(const Instance& ins, const Solution& start, Options& opt) {
    AdaptResult res;
    Solution s = start;
    Ils probe(ins, opt.seed, opt.ils, opt.ls, opt.ruin);
    probe.descend(s);
    int delayed = 0;
    for (int f = 0; f < ins.n; ++f) delayed += s.startOf[f] > ins.r[f];
    res.delayedFrac = ins.n > 0 ? static_cast<double>(delayed) / ins.n : 0.0;
    res.wide = res.delayedFrac >= kWideThreshold;
    if (!opt.adapt || opt.windowsSet) return res;
    res.applied = true;
    if (res.wide) {
        opt.ls.posWindow = kWidePosWindow;
        opt.ruin.insertWindow = kWideInsertWindow;
        opt.ruin.maxString = kWideMaxString;
    }
    return res;
}
