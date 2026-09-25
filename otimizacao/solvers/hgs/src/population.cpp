#include "population.hpp"

#include <algorithm>
#include <cmath>

Individual::Individual(const Instance& ins, Routes rts) : routes(std::move(rts)) {
    cost = evaluateRoutes(ins, routes, &S);
    succ.assign(ins.n, -1);
    pred.assign(ins.n, -1);
    for (const auto& rw : routes)
        for (size_t q = 0; q + 1 < rw.size(); ++q) {
            succ[rw[q]] = rw[q + 1];
            pred[rw[q + 1]] = rw[q];
        }
}

double Individual::avgClosest(int k) const {
    int cnt = std::min<int>(k, (int)proximity.size());
    if (cnt == 0) return 0.0;
    double s = 0;
    for (int i = 0; i < cnt; ++i) s += proximity[i].first;
    return s / cnt;
}

double brokenPairsDistance(const Individual& a, const Individual& b) {
    const int n = (int)a.succ.size();
    int diff = 0;
    for (int x = 0; x < n; ++x) diff += a.succ[x] != b.succ[x];
    return (double)diff / n;
}

double startsDistance(const Individual& a, const Individual& b) {
    const int n = (int)a.S.size();
    int diff = 0;
    for (int x = 0; x < n; ++x) diff += a.S[x] != b.S[x];
    return (double)diff / n;
}

void Population::add(const Individual& ind) {
    auto np = std::make_unique<Individual>(ind);
    np->proximity.clear();
    for (auto& o : pop_) {
        double d = P.dist == "arcs"     ? brokenPairsDistance(*o, *np)
                   : P.dist == "starts" ? startsDistance(*o, *np)
                   : (startsDistance(*o, *np) > 0 ? 0.5 * brokenPairsDistance(*o, *np) + startsDistance(*o, *np) : 0.0);
        auto insertSorted = [](std::vector<std::pair<double, Individual*>>& v, std::pair<double, Individual*> e) {
            v.insert(std::upper_bound(v.begin(), v.end(), e,
                                      [](const auto& x, const auto& y) { return x.first < y.first; }),
                     e);
        };
        insertSorted(o->proximity, {d, np.get()});
        insertSorted(np->proximity, {d, o.get()});
    }
    pop_.push_back(std::move(np));
    if ((int)pop_.size() > P.mu + P.lambda)
        while ((int)pop_.size() > P.mu) removeWorst();
}

void Population::updateBiasedFitness() {
    const int sz = (int)pop_.size();
    if (sz == 1) {
        pop_[0]->biasedFitness = 0;
        return;
    }
    std::vector<std::pair<double, int>> div(sz);
    for (int i = 0; i < sz; ++i) div[i] = {-pop_[i]->avgClosest(P.nbClose), i};
    std::sort(div.begin(), div.end());
    std::vector<double> divRank(sz);
    for (int k = 0; k < sz; ++k) divRank[div[k].second] = (double)k / (sz - 1);
    std::vector<std::pair<long long, int>> fit(sz);
    for (int i = 0; i < sz; ++i) fit[i] = {pop_[i]->cost, i};
    std::sort(fit.begin(), fit.end());
    const double eliteFactor = 1.0 - (double)P.nbElite / sz;
    for (int k = 0; k < sz; ++k) {
        int i = fit[k].second;
        double fitRank = (double)k / (sz - 1);
        pop_[i]->biasedFitness = fitRank + eliteFactor * divRank[i];
    }
}

void Population::removeWorst() {
    updateBiasedFitness();
    const Individual* b = best();  // the best-cost individual is never removed
    int worst = -1;
    bool worstIsClone = false;
    double worstFit = -1;
    for (int i = 0; i < (int)pop_.size(); ++i) {
        if (pop_[i].get() == b) continue;
        bool clone = pop_[i]->avgClosest(1) < 1e-9;
        if (worst < 0 || (clone && !worstIsClone) ||
            (clone == worstIsClone && pop_[i]->biasedFitness > worstFit)) {
            worst = i;
            worstIsClone = clone;
            worstFit = pop_[i]->biasedFitness;
        }
    }
    if (worst < 0) return;
    Individual* dead = pop_[worst].get();
    for (auto& o : pop_) {
        auto& v = o->proximity;
        v.erase(std::remove_if(v.begin(), v.end(), [dead](const auto& e) { return e.second == dead; }), v.end());
    }
    pop_.erase(pop_.begin() + worst);
}

const Individual& Population::tournament(Rng& rng) {
    updateBiasedFitness();
    const Individual& a = *pop_[rng.uniform((int)pop_.size())];
    const Individual& b = *pop_[rng.uniform((int)pop_.size())];
    return a.biasedFitness < b.biasedFitness ? a : b;
}

const Individual* Population::best() const {
    const Individual* b = nullptr;
    for (const auto& o : pop_)
        if (!b || o->cost < b->cost) b = o.get();
    return b;
}

double Population::averageCost() const {
    if (pop_.empty()) return 0;
    double s = 0;
    for (const auto& o : pop_) s += (double)o->cost;
    return s / pop_.size();
}

double Population::averageDiversity() const {
    if (pop_.empty()) return 0;
    double s = 0;
    for (const auto& o : pop_) s += o->avgClosest(P.nbClose);
    return s / pop_.size();
}
