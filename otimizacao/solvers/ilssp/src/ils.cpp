#include "ils.hpp"

#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <numeric>
#include <string>

namespace {

double nowSec() {
    using namespace std::chrono;
    return duration<double>(steady_clock::now().time_since_epoch()).count();
}

void failSelfTest(const std::string& what) {
    std::fprintf(stderr, "SELFTEST FAILURE: %s\n", what.c_str());
    std::abort();
}

void submitAll(const Solution& s, ColumnPool* pool, long long stamp, IlsStats& st) {
    if (!pool) return;
    for (const Runway& R : s.rw) {
        if (R.seq.empty()) continue;
        bool isNew = false;
        pool->add(R.seq, R.cost(), 0, stamp, &isNew);
        st.newColumns += isNew;
    }
}

}  // namespace

Ils::Ils(const Instance& ins, uint64_t seed, IlsParams ip, LsParams lp, RuinParams rp)
    : ins_(ins), rng_(seed), ip_(ip), ls_(ins, rng_, lp), rr_(ins, rng_, rp) {
    journal_.init(ins.m);
    meanP_ = std::accumulate(ins.p.begin(), ins.p.end(), 0.0) / ins.n;
}

void Ils::descend(Solution& s) {
    ls_.attach(&s, nullptr);
    ls_.activateAll();
    ls_.run();
}

void Ils::run(Solution& cur, Solution& best, double seconds, ColumnPool* pool, long long stamp,
              IlsStats& st, const std::function<void(const Solution&)>& onBest) {
    const double t0 = nowSec();
    const double tEnd = t0 + seconds;
    ls_.attach(&cur, &journal_);
    journal_.init(ins_.m);
    ls_.activateAll();
    ls_.run();
    journal_.commit();
    if (cur.cost < best.cost) {
        best = cur;
        onBest(best);
    }
    submitAll(cur, pool, stamp, st);
    long long curCost = cur.cost;
    double T = ip_.T0 * meanP_;
    const double logRatio = std::log(ip_.Tf / ip_.T0);
    std::string err;
    for (long long it = 0;; ++it) {
        if ((it & 31) == 0) {
            const double now = nowSec();
            if (now >= tEnd || (ip_.stop && ip_.stop->load(std::memory_order_relaxed))) break;
            const double frac = (now - t0) / seconds;
            T = ip_.T0 * meanP_ * std::exp(logRatio * frac);
        }
        ++st.iterations;
        regions_.clear();
        rr_.perturb(ls_.editor(), regions_);
        ls_.activate(regions_);
        st.lsMoves += ls_.run();
        const long long newCost = cur.cost;
        if (ip_.selfTest && !validateSolution(ins_, cur, err)) failSelfTest(err);
        const long long d = newCost - curCost;
        const bool accept = d <= 0 || rng_.uniform() < std::exp(-static_cast<double>(d) / T);
        if (accept) {
            if (pool && newCost <= best.cost * (1.0 + ip_.poolTol)) {
                for (int k : journal_.touched()) {
                    const Runway& R = cur.rw[k];
                    if (R.seq.empty()) continue;
                    bool isNew = false;
                    pool->add(R.seq, R.cost(), 0, stamp, &isNew);
                    st.newColumns += isNew;
                }
            }
            journal_.commit();
            curCost = newCost;
            ++st.accepted;
            if (newCost < best.cost) {
                best = cur;
                submitAll(best, pool, stamp, st);
                onBest(best);
            }
        } else {
            journal_.revert(ins_, cur);
            if (ip_.selfTest && (cur.cost != curCost || !validateSolution(ins_, cur, err)))
                failSelfTest("revert: " + err);
        }
    }
}
