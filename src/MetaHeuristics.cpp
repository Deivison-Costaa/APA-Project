#include "MetaHeuristics.hpp"
#include "GreedyAlgorithm.hpp"
#include "VariableNeighborhoodDescent.hpp"

#include <omp.h>
#include <algorithm>
#include <chrono>
#include <iostream>
#include <limits>
#include <numeric>
#include <vector>

namespace
{
    using Clock = std::chrono::steady_clock;

    class Timer
    {
    public:
        explicit Timer(double limitSeconds) : begin(Clock::now()), limit(limitSeconds) {}
        double elapsed() const { return std::chrono::duration<double>(Clock::now() - begin).count(); }
        bool expired() const { return limit > 0.0 && elapsed() >= limit; }

    private:
        Clock::time_point begin;
        double limit;
    };

    unsigned resolveSeed(unsigned seed)
    {
        return seed != 0 ? seed : std::random_device{}();
    }

    int randomInt(std::mt19937 &gen, int lo, int hi)
    {
        return std::uniform_int_distribution<int>(lo, hi)(gen);
    }

    // Tamanho máximo de um segmento da perturbação: ceil(|pista| / 10), como no double bridge
    int maxSegment(int length)
    {
        return std::max(1, (length + 9) / 10);
    }

    // Índices das pistas que satisfazem `pred`
    template <typename Pred>
    std::vector<int> runwaysWhere(const Schedule &solution, Pred pred)
    {
        std::vector<int> result;
        for (int r = 0; r < static_cast<int>(solution.size()); ++r)
            if (pred(solution[r]))
                result.push_back(r);
        return result;
    }
}

MetaHeuristics::MetaHeuristics(const Instance &inst) : instance(inst) {}

// ---------------------------------------------------------------------------
// Perturbação
// ---------------------------------------------------------------------------

bool MetaHeuristics::doubleBridgeIntra(Schedule &solution, std::mt19937 &gen) const
{
    auto candidates = runwaysWhere(solution, [](const std::vector<int> &r)
                                   { return r.size() >= 2; });
    if (candidates.empty())
        return false;

    auto &runway = solution[candidates[randomInt(gen, 0, static_cast<int>(candidates.size()) - 1)]];
    const int len = static_cast<int>(runway.size());
    const int maxSize = std::min(maxSegment(len), len / 2);

    // Dois segmentos não sobrepostos trocam de posição
    const int size1 = randomInt(gen, 1, maxSize);
    const int size2 = randomInt(gen, 1, maxSize);
    const int start1 = randomInt(gen, 0, len - size1 - size2);
    const int start2 = randomInt(gen, start1 + size1, len - size2);

    std::vector<int> middle;
    middle.reserve(start2 + size2 - start1);
    middle.insert(middle.end(), runway.begin() + start2, runway.begin() + start2 + size2);
    middle.insert(middle.end(), runway.begin() + start1 + size1, runway.begin() + start2);
    middle.insert(middle.end(), runway.begin() + start1, runway.begin() + start1 + size1);
    std::copy(middle.begin(), middle.end(), runway.begin() + start1);
    return true;
}

bool MetaHeuristics::swapSegmentsInter(Schedule &solution, std::mt19937 &gen) const
{
    auto candidates = runwaysWhere(solution, [](const std::vector<int> &r)
                                   { return !r.empty(); });
    if (candidates.size() < 2)
        return false;

    std::shuffle(candidates.begin(), candidates.end(), gen);
    auto &a = solution[candidates[0]];
    auto &b = solution[candidates[1]];

    const int sizeA = randomInt(gen, 1, maxSegment(static_cast<int>(a.size())));
    const int sizeB = randomInt(gen, 1, maxSegment(static_cast<int>(b.size())));
    const int startA = randomInt(gen, 0, static_cast<int>(a.size()) - sizeA);
    const int startB = randomInt(gen, 0, static_cast<int>(b.size()) - sizeB);

    std::vector<int> segA(a.begin() + startA, a.begin() + startA + sizeA);
    std::vector<int> segB(b.begin() + startB, b.begin() + startB + sizeB);
    a.erase(a.begin() + startA, a.begin() + startA + sizeA);
    a.insert(a.begin() + startA, segB.begin(), segB.end());
    b.erase(b.begin() + startB, b.begin() + startB + sizeB);
    b.insert(b.begin() + startB, segA.begin(), segA.end());
    return true;
}

bool MetaHeuristics::relocateSegmentInter(Schedule &solution, std::mt19937 &gen) const
{
    const int m = static_cast<int>(solution.size());
    auto sources = runwaysWhere(solution, [](const std::vector<int> &r)
                                { return !r.empty(); });
    if (m < 2 || sources.empty())
        return false;

    const int src = sources[randomInt(gen, 0, static_cast<int>(sources.size()) - 1)];
    int tgt = randomInt(gen, 0, m - 2);
    if (tgt >= src)
        ++tgt;

    auto &source = solution[src];
    auto &target = solution[tgt];
    const int size = randomInt(gen, 1, maxSegment(static_cast<int>(source.size())));
    const int from = randomInt(gen, 0, static_cast<int>(source.size()) - size);
    const int to = randomInt(gen, 0, static_cast<int>(target.size()));

    target.insert(target.begin() + to, source.begin() + from, source.begin() + from + size);
    source.erase(source.begin() + from, source.begin() + from + size);
    return true;
}

void MetaHeuristics::perturb(Schedule &solution, int strength, std::mt19937 &gen) const
{
    for (int s = 0; s < strength; ++s)
    {
        int order[3] = {0, 1, 2};
        std::shuffle(order, order + 3, gen);
        for (int move : order)
        {
            bool applied = (move == 0)   ? doubleBridgeIntra(solution, gen)
                           : (move == 1) ? swapSegmentsInter(solution, gen)
                                         : relocateSegmentInter(solution, gen);
            if (applied)
                break;
        }
    }
}

// ---------------------------------------------------------------------------
// ILS
// ---------------------------------------------------------------------------

Schedule MetaHeuristics::ils(const IlsParameters &params, const Schedule &initialSolution)
{
    const int n = instance.numberOfFlights;
    const int maxIterIls = params.maxIterIls >= 0 ? params.maxIterIls : (n >= 150 ? n / 2 : 10 * n);
    const int maxStrength = std::max(1, params.maxStrength);
    const int threads = params.threads > 0 ? params.threads : omp_get_max_threads();
    const unsigned baseSeed = resolveSeed(params.seed);
    const Timer timer(params.timeLimit);

    Schedule bestOfAll;
    long long bestOfAllCost = std::numeric_limits<long long>::max();

#pragma omp parallel num_threads(threads)
    {
        std::mt19937 gen(baseSeed + 7919u * static_cast<unsigned>(omp_get_thread_num()));
        VariableNeighborhoodDescent localSearch(instance, gen());
        GreedyAlgorithm greedy;
        std::uniform_real_distribution<double> alphaDist(0.0, 0.25);

#pragma omp for schedule(dynamic)
        for (int iter = 0; iter < params.maxIter; ++iter)
        {
            if (timer.expired())
                continue;

            Schedule s = (iter == 0 && !initialSolution.empty())
                             ? initialSolution
                             : greedy.graspNearestNeighbor(instance, alphaDist(gen), gen);
            long long cost = localSearch.rvnd(s);
            Schedule best = s;
            long long bestCost = cost;

            int iterIls = 0;
            while (iterIls < maxIterIls && !timer.expired())
            {
                // A força da perturbação cresce com a estagnação
                const int strength = randomInt(gen, 1, 1 + (maxStrength - 1) * iterIls / std::max(1, maxIterIls));
                s = best;
                perturb(s, strength, gen);
                cost = localSearch.rvnd(s);

                if (cost < bestCost)
                {
                    best = s;
                    bestCost = cost;
                    iterIls = 0;
                }
                else
                {
                    ++iterIls;
                }
            }

#pragma omp critical(ils_best)
            {
                if (bestCost < bestOfAllCost)
                {
                    bestOfAllCost = bestCost;
                    bestOfAll = best;
                    if (params.verbose)
                        std::cout << "  [ILS] reinicio " << iter << ": melhor custo = " << bestOfAllCost
                                  << " (" << timer.elapsed() << " s)\n";
                }
            }
        }
    }

    return bestOfAll;
}

// ---------------------------------------------------------------------------
// LNS
// ---------------------------------------------------------------------------

std::vector<int> MetaHeuristics::destroy(Schedule &solution, int k, std::mt19937 &gen) const
{
    std::vector<int> all;
    all.reserve(instance.numberOfFlights);
    for (const auto &runway : solution)
        all.insert(all.end(), runway.begin(), runway.end());

    k = std::min(k, static_cast<int>(all.size()));
    if (k <= 0)
        return {};

    // Fisher-Yates parcial: só os k primeiros precisam ser sorteados
    for (int i = 0; i < k; ++i)
        std::swap(all[i], all[randomInt(gen, i, static_cast<int>(all.size()) - 1)]);
    all.resize(k);

    std::vector<char> removed(instance.numberOfFlights, 0);
    for (int f : all)
        removed[f] = 1;
    for (auto &runway : solution)
        runway.erase(std::remove_if(runway.begin(), runway.end(), [&](int f)
                                    { return removed[f]; }),
                     runway.end());
    return all;
}

Schedule MetaHeuristics::lns(const LnsParameters &params, const Schedule &initialSolution)
{
    std::mt19937 gen(resolveSeed(params.seed));
    VariableNeighborhoodDescent localSearch(instance, gen());
    GreedyAlgorithm greedy;
    const Timer timer(params.timeLimit);

    Schedule current = initialSolution.empty() ? greedy.graspNearestNeighbor(instance, 0.01, gen)
                                               : initialSolution;
    long long currentCost = localSearch.rvnd(current);
    Schedule best = current;
    long long bestCost = currentCost;

    int k = params.minDestroy;
    int withoutImprovement = 0;

    for (int iter = 0; iter < params.maxIterations && !timer.expired(); ++iter)
    {
        Schedule s = current;
        const auto removed = destroy(s, k, gen);
        localSearch.insertCheapest(s, removed);
        const long long cost = localSearch.rvnd(s);

        if (cost < currentCost)
        {
            current = std::move(s);
            currentCost = cost;
            withoutImprovement = 0;
            k = params.minDestroy;

            if (currentCost < bestCost)
            {
                best = current;
                bestCost = currentCost;
                if (params.verbose)
                    std::cout << "  [LNS] iter " << iter << ": melhor custo = " << bestCost
                              << " (" << timer.elapsed() << " s)\n";
            }
        }
        else if (++withoutImprovement >= params.patience)
        {
            withoutImprovement = 0;
            k += params.destroyStep;
            if (k > params.maxDestroy)
            {
                current = greedy.graspNearestNeighbor(instance, 0.01, gen);
                currentCost = localSearch.rvnd(current);
                k = params.minDestroy;
            }
        }
    }

    return best;
}
