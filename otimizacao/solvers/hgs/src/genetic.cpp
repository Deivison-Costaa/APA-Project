#include "genetic.hpp"

#include <algorithm>
#include <cstdio>

#include "local_search.hpp"
#include "operators.hpp"
#include "population.hpp"

bool SharedBest::offer(const Routes& routes, long long cost, int thread) {
    std::lock_guard<std::mutex> lk(mu_);
    if (cost >= cost_.load()) return false;
    cost_.store(cost);
    routes_ = routes;
    dirty_ = true;
    double now = elapsed();
    std::printf("[%8.2fs] T%d new best %lld\n", now, thread, cost);
    std::fflush(stdout);
    if (now - lastWrite_ > 2.0) {
        writeSolution(I, routes_, cost, out_);
        dirty_ = false;
        lastWrite_ = now;
    }
    return true;
}

Routes SharedBest::routes() {
    std::lock_guard<std::mutex> lk(mu_);
    return routes_;
}

void SharedBest::flush() {
    std::lock_guard<std::mutex> lk(mu_);
    if (dirty_ && !routes_.empty()) {
        if (!writeSolution(I, routes_, cost_.load(), out_))
            std::fprintf(stderr, "ERROR: cannot write %s\n", out_.c_str());
        dirty_ = false;
        lastWrite_ = elapsed();
    }
}

namespace {

class Island {
public:
    Island(const Instance& ins, const Params& par, uint64_t seed, int id, double deadline, SharedBest& shared)
        : I(ins), P(par), rng(seed), id_(id), deadline_(deadline), shared_(shared), ls(ins, par), pop(ins, par) {}

    GeneticStats run() {
        initPopulation(true);
        double nextLog = shared_.elapsed() + 5.0;
        while (shared_.elapsed() < deadline_) {
            const Individual& A = pop.tournament(rng);
            const Individual* B = &pop.tournament(rng);
            for (int tries = 0; B == &A && tries < 5; ++tries) B = &pop.tournament(rng);
            Offspring off = makeOffspring(A, *B);
            Individual child = educate(off, &A, B);
            ++st.generations;
            ++st.kindCount[off.kind];
            if (child.cost < std::min(A.cost, B->cost)) ++st.kindBetterParents[off.kind];
            if (child.cost < bestRun_) ++st.kindNewBest[off.kind];
            ++itNoImp;
            if (child.cost < bestRun_) {
                bestRun_ = child.cost;
                itNoImp = 0;
            }
            if (child.cost < st.bestCost) {
                st.bestCost = child.cost;
                shared_.offer(child.routes, child.cost, id_);
            }
            pop.add(child);
            if (itNoImp > P.itNoImprove) restart();
            if (P.verbose && shared_.elapsed() >= nextLog) {
                log();
                nextLog += 5.0;
            }
        }
        st.lsMoves = ls.movesApplied();
        st.lsEvals = ls.evaluations();
        return st;
    }

private:
    const Instance& I;
    const Params& P;
    Rng rng;
    int id_;
    double deadline_;
    SharedBest& shared_;
    LocalSearch ls;
    Population pop;
    GeneticStats st;
    long long itNoImp = 0;
    std::vector<char> dirty_;
    std::vector<Individual> archive_;  // best individual of each finished run
    long long bestRun_ = LLONG_MAX;  // best cost since the last restart

    // Flights whose (pred, succ, start) match neither parent are "dirty"; the descent
    // starts around them only (parents are local optima already).
    Individual educate(Offspring& off, const Individual* A = nullptr, const Individual* B = nullptr) {
        ls.load(off.routes);
        std::shuffle(off.missing.begin(), off.missing.end(), rng);
        if (rng.real() < P.pSortedInsert)
            std::stable_sort(off.missing.begin(), off.missing.end(),
                             [&](int a, int b) { return I.r[a] < I.r[b]; });
        ls.insertMissing(off.missing);
        if (A && P.focused) {
            const auto& pr = ls.preds();
            const auto& sc = ls.succs();
            const auto& S = ls.starts();
            auto same = [&](const Individual* X, int x) {
                return X && X->pred[x] == pr[x] && X->succ[x] == sc[x] && X->S[x] == S[x];
            };
            dirty_.assign(I.n, 0);
            for (int x = 0; x < I.n; ++x) dirty_[x] = !(same(A, x) || same(B, x));
            ls.run(rng, &dirty_);
        } else {
            ls.run(rng);
        }
        Individual ind(I, ls.routes());
        if (ind.cost != ls.cost()) {
            std::fprintf(stderr, "BUG: incremental cost %lld != full cost %lld\n", ls.cost(), ind.cost);
            std::abort();
        }
        return ind;
    }

    Offspring makeOffspring(const Individual& A, const Individual& B) {
        const std::string& x = P.xover;
        if (x == "tw") return crossoverTimeWindow(I, P, A, B, rng);
        if (x == "rx") return crossoverRunways(I, A, B, rng);
        if (x == "ruin") return ruinWindow(I, P, A, rng);
        if (x == "none") return Offspring{randomGreedy(I, rng), {}};
        // mix
        if (rng.real() < P.pTw) return crossoverTimeWindow(I, P, A, B, rng);
        return ruinWindow(I, P, A, rng);
    }

    void addAndTrack(const Individual& ind) {
        bestRun_ = std::min(bestRun_, ind.cost);
        if (ind.cost < st.bestCost) {
            st.bestCost = ind.cost;
            shared_.offer(ind.routes, ind.cost, id_);
        }
        pop.add(ind);
    }

    void initPopulation(bool first) {
        if (first && !P.initFile.empty()) {
            Offspring off{readSolution(I, P.initFile), {}};
            Individual ind = educate(off);
            std::printf("[%8.2fs] T%d init solution educated: %lld\n", shared_.elapsed(), id_, ind.cost);
            addAndTrack(ind);
        } else if (!first && P.keepBest && shared_.cost() < LLONG_MAX) {
            addAndTrack(Individual(I, shared_.routes()));
        }
        int nRandom = P.initPop;
        if (!first && P.eliteEvery > 0 && st.restarts % P.eliteEvery == 0 && archive_.size() >= 2) {
            // elite restart: recombine the best solutions of earlier runs
            for (const auto& e : archive_) addAndTrack(e);
            // islands share their best through the global best (a no-op with one thread)
            if (shared_.cost() < archive_.front().cost) addAndTrack(Individual(I, shared_.routes()));
            nRandom = std::max(0, P.initPop - (int)archive_.size()) / 2;
            if (P.verbose)
                std::printf("[%8.2fs] T%d elite restart with %zu archived run bests\n", shared_.elapsed(), id_,
                            archive_.size());
        }
        const double budget = std::min(deadline_, shared_.elapsed() + P.initFrac * P.timeLimit);
        for (int i = 0; i < nRandom; ++i) {
            if (pop.size() >= 2 && shared_.elapsed() > budget) break;
            Offspring off{randomGreedy(I, rng), {}};
            addAndTrack(educate(off));
        }
        itNoImp = 0;
        if (P.verbose) log();
    }

    void restart() {
        ++st.restarts;
        if (P.verbose)
            std::printf("[%8.2fs] T%d restart #%lld (pop best %lld)\n", shared_.elapsed(), id_, st.restarts,
                        pop.best() ? pop.best()->cost : -1LL);
        archiveRunBest();
        pop.clear();
        bestRun_ = LLONG_MAX;
        initPopulation(false);
    }

    // Keeps the best individual of the finished run (distinct start vectors only).
    void archiveRunBest() {
        const Individual* b = pop.best();
        if (!b) return;
        for (auto& e : archive_)
            if (startsDistance(e, *b) == 0.0) return;
        archive_.push_back(*b);
        archive_.back().proximity.clear();
        std::sort(archive_.begin(), archive_.end(),
                  [](const Individual& x, const Individual& y) { return x.cost < y.cost; });
        if ((int)archive_.size() > P.archiveSize) archive_.pop_back();
    }

    void log() {
        const Individual* b = pop.best();
        std::printf("[%8.2fs] T%d gen %lld pop %d best %lld avg %.1f div %.3f | global %lld | moves %lld evals %lld\n",
                    shared_.elapsed(), id_, st.generations, pop.size(), b ? b->cost : -1LL, pop.averageCost(),
                    pop.averageDiversity(), shared_.cost(), ls.movesApplied(), ls.evaluations());
        std::fflush(stdout);
    }
};

}  // namespace

GeneticStats runGenetic(const Instance& ins, const Params& par, uint64_t seed, int threadId, double deadline,
                        SharedBest& shared) {
    Island island(ins, par, seed, threadId, deadline, shared);
    return island.run();
}
