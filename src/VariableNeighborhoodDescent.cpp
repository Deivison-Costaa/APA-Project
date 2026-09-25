#include "VariableNeighborhoodDescent.hpp"

#include <algorithm>
#include <limits>
#include <utility>
#include <vector>

namespace
{
    constexpr long long INF = std::numeric_limits<long long>::max();
}

VariableNeighborhoodDescent::VariableNeighborhoodDescent(const Instance &inst, unsigned seed)
    : instance(inst), gen(seed) {}

void VariableNeighborhoodDescent::load(Schedule &s)
{
    solution = &s;
    states.resize(s.size());
    cost = 0;
    for (int r = 0; r < static_cast<int>(s.size()); ++r)
    {
        rebuild(r);
        cost += states[r].total;
    }
}

void VariableNeighborhoodDescent::rebuild(int r)
{
    const auto &runway = (*solution)[r];
    auto &st = states[r];
    const int len = static_cast<int>(runway.size());
    st.start.resize(len);
    st.prefix.resize(len);

    int prevEnd = 0, prev = -1;
    long long acc = 0;
    for (int k = 0; k < len; ++k)
    {
        const int f = runway[k];
        const int release = instance.landingTakeoffTime[f];
        const int s = (prev < 0) ? release : std::max(release, prevEnd + instance.separation(prev, f));
        acc += static_cast<long long>(s - release) * instance.penalties[f];
        st.start[k] = s;
        st.prefix[k] = acc;
        prevEnd = s + instance.waitingTime[f];
        prev = f;
    }
    st.total = acc;
}

long long VariableNeighborhoodDescent::evaluate(int r, int prefixLength, const Piece *pieces, int numPieces,
                                                int tailStart, long long limit) const
{
    const auto &runway = (*solution)[r];
    const auto &st = states[r];
    const auto &release = instance.landingTakeoffTime;
    const auto &duration = instance.waitingTime;
    const auto &penalty = instance.penalties;

    int prev = -1, prevEnd = 0;
    long long c = 0;
    if (prefixLength > 0)
    {
        prev = runway[prefixLength - 1];
        prevEnd = st.start[prefixLength - 1] + duration[prev];
        c = st.prefix[prefixLength - 1];
        if (c >= limit)
            return limit;
    }

    // Trechos que mudaram de posição
    for (int p = 0; p < numPieces; ++p)
    {
        const int *flights = pieces[p].flights;
        for (int k = 0; k < pieces[p].length; ++k)
        {
            const int f = flights[k];
            const int s = (prev < 0) ? release[f] : std::max(release[f], prevEnd + instance.separation(prev, f));
            c += static_cast<long long>(s - release[f]) * penalty[f];
            if (c >= limit)
                return limit;
            prevEnd = s + duration[f];
            prev = f;
        }
    }

    // Sufixo original da pista
    const int len = static_cast<int>(runway.size());
    for (int q = tailStart; q < len; ++q)
    {
        const int f = runway[q];
        const int s = (prev < 0) ? release[f] : std::max(release[f], prevEnd + instance.separation(prev, f));

        // Mesmo antecessor e mesmo horário de início: daqui em diante nada muda
        const int originalPrev = (q > 0) ? runway[q - 1] : -1;
        if (prev == originalPrev && s == st.start[q])
            return std::min(limit, c + st.total - prefixCost(r, q));

        c += static_cast<long long>(s - release[f]) * penalty[f];
        if (c >= limit)
            return limit;
        prevEnd = s + duration[f];
        prev = f;
    }
    return c;
}

bool VariableNeighborhoodDescent::swapIntra()
{
    long long bestDelta = 0;
    int bestR = -1, bestI = 0, bestJ = 0;

    for (int r = 0; r < static_cast<int>(solution->size()); ++r)
    {
        const auto &runway = (*solution)[r];
        const int len = static_cast<int>(runway.size());
        const long long total = states[r].total;

        for (int i = 0; i + 1 < len; ++i)
        {
            // Nenhuma troca a partir da posição i pode ficar abaixo do custo do prefixo
            if (prefixCost(r, i) >= total + bestDelta)
                break;

            for (int j = i + 1; j < len; ++j)
            {
                const Piece pieces[3] = {{&runway[j], 1}, {runway.data() + i + 1, j - i - 1}, {&runway[i], 1}};
                const long long limit = total + bestDelta;
                const long long c = evaluate(r, i, pieces, 3, j + 1, limit);
                if (c < limit)
                {
                    bestDelta = c - total;
                    bestR = r, bestI = i, bestJ = j;
                }
            }
        }
    }

    if (bestR < 0)
        return false;

    std::swap((*solution)[bestR][bestI], (*solution)[bestR][bestJ]);
    rebuild(bestR);
    cost += bestDelta;
    return true;
}

bool VariableNeighborhoodDescent::swapInter()
{
    long long bestDelta = 0;
    int bestR1 = -1, bestI = 0, bestR2 = 0, bestJ = 0;
    const int m = static_cast<int>(solution->size());

    for (int r1 = 0; r1 < m; ++r1)
    {
        const auto &a = (*solution)[r1];
        for (int r2 = r1 + 1; r2 < m; ++r2)
        {
            const auto &b = (*solution)[r2];
            const long long totalAB = states[r1].total + states[r2].total;

            for (int i = 0; i < static_cast<int>(a.size()); ++i)
            {
                const long long prefixA = prefixCost(r1, i);
                if (prefixA >= totalAB + bestDelta)
                    break;

                const Piece intoB{&a[i], 1};
                for (int j = 0; j < static_cast<int>(b.size()); ++j)
                {
                    const long long bound = totalAB + bestDelta;
                    const long long prefixB = prefixCost(r2, j);
                    if (prefixA + prefixB >= bound)
                        break;

                    const Piece intoA{&b[j], 1};
                    const long long limitA = bound - prefixB;
                    const long long costA = evaluate(r1, i, &intoA, 1, i + 1, limitA);
                    if (costA >= limitA)
                        continue;

                    const long long limitB = bound - costA;
                    const long long costB = evaluate(r2, j, &intoB, 1, j + 1, limitB);
                    if (costB >= limitB)
                        continue;

                    bestDelta = costA + costB - totalAB;
                    bestR1 = r1, bestI = i, bestR2 = r2, bestJ = j;
                }
            }
        }
    }

    if (bestR1 < 0)
        return false;

    std::swap((*solution)[bestR1][bestI], (*solution)[bestR2][bestJ]);
    rebuild(bestR1);
    rebuild(bestR2);
    cost += bestDelta;
    return true;
}

bool VariableNeighborhoodDescent::orOptIntra(int k)
{
    long long bestDelta = 0;
    int bestR = -1, bestI = 0, bestP = 0;

    for (int r = 0; r < static_cast<int>(solution->size()); ++r)
    {
        const auto &runway = (*solution)[r];
        const int len = static_cast<int>(runway.size());
        const long long total = states[r].total;
        if (len <= k)
            continue;

        for (int i = 0; i + k <= len; ++i)
        {
            const Piece block{runway.data() + i, k};

            // Bloco movido para antes da posição p < i
            for (int p = 0; p < i; ++p)
            {
                const long long limit = total + bestDelta;
                if (prefixCost(r, p) >= limit)
                    break;
                const Piece pieces[2] = {block, {runway.data() + p, i - p}};
                const long long c = evaluate(r, p, pieces, 2, i + k, limit);
                if (c < limit)
                {
                    bestDelta = c - total;
                    bestR = r, bestI = i, bestP = p;
                }
            }

            // Bloco movido para antes da posição p > i + k (p == len: fim da pista)
            if (prefixCost(r, i) >= total + bestDelta)
                continue;
            for (int p = i + k + 1; p <= len; ++p)
            {
                const long long limit = total + bestDelta;
                const Piece pieces[2] = {{runway.data() + i + k, p - i - k}, block};
                const long long c = evaluate(r, i, pieces, 2, p, limit);
                if (c < limit)
                {
                    bestDelta = c - total;
                    bestR = r, bestI = i, bestP = p;
                }
            }
        }
    }

    if (bestR < 0)
        return false;

    auto &runway = (*solution)[bestR];
    std::vector<int> block(runway.begin() + bestI, runway.begin() + bestI + k);
    runway.erase(runway.begin() + bestI, runway.begin() + bestI + k);
    const int insertAt = (bestP < bestI) ? bestP : bestP - k;
    runway.insert(runway.begin() + insertAt, block.begin(), block.end());
    rebuild(bestR);
    cost += bestDelta;
    return true;
}

bool VariableNeighborhoodDescent::orOptInter(int k)
{
    long long bestDelta = 0;
    int bestSrc = -1, bestI = 0, bestTgt = 0, bestP = 0;
    const int m = static_cast<int>(solution->size());

    for (int src = 0; src < m; ++src)
    {
        const auto &source = (*solution)[src];
        const int lenS = static_cast<int>(source.size());

        for (int i = 0; i + k <= lenS; ++i)
        {
            // Custo da pista de origem sem o bloco: calculado uma única vez por bloco
            const long long costSource = evaluate(src, i, nullptr, 0, i + k, INF);
            const Piece block{source.data() + i, k};

            for (int tgt = 0; tgt < m; ++tgt)
            {
                if (tgt == src)
                    continue;
                const long long totalST = states[src].total + states[tgt].total;
                const int lenT = static_cast<int>((*solution)[tgt].size());

                for (int p = 0; p <= lenT; ++p)
                {
                    const long long bound = totalST + bestDelta - costSource;
                    if (prefixCost(tgt, p) >= bound)
                        break;
                    const long long c = evaluate(tgt, p, &block, 1, p, bound);
                    if (c < bound)
                    {
                        bestDelta = costSource + c - totalST;
                        bestSrc = src, bestI = i, bestTgt = tgt, bestP = p;
                    }
                }
            }
        }
    }

    if (bestSrc < 0)
        return false;

    auto &source = (*solution)[bestSrc];
    auto &target = (*solution)[bestTgt];
    std::vector<int> block(source.begin() + bestI, source.begin() + bestI + k);
    source.erase(source.begin() + bestI, source.begin() + bestI + k);
    target.insert(target.begin() + bestP, block.begin(), block.end());
    rebuild(bestSrc);
    rebuild(bestTgt);
    cost += bestDelta;
    return true;
}

std::vector<VariableNeighborhoodDescent::Neighborhood> VariableNeighborhoodDescent::availableNeighborhoods() const
{
    std::vector<Neighborhood> list = {SwapIntra, OrOpt1Intra, OrOpt2Intra, OrOpt3Intra};
    if (solution->size() > 1)
        list.insert(list.end(), {SwapInter, OrOpt1Inter, OrOpt2Inter, OrOpt3Inter});
    return list;
}

bool VariableNeighborhoodDescent::explore(Neighborhood n)
{
    switch (n)
    {
    case SwapIntra:
        return swapIntra();
    case SwapInter:
        return swapInter();
    case OrOpt1Intra:
        return orOptIntra(1);
    case OrOpt2Intra:
        return orOptIntra(2);
    case OrOpt3Intra:
        return orOptIntra(3);
    case OrOpt1Inter:
        return orOptInter(1);
    case OrOpt2Inter:
        return orOptInter(2);
    case OrOpt3Inter:
        return orOptInter(3);
    default:
        return false;
    }
}

long long VariableNeighborhoodDescent::vnd(Schedule &s)
{
    load(s);
    const auto neighborhoods = availableNeighborhoods();

    std::size_t k = 0;
    while (k < neighborhoods.size())
    {
        if (explore(neighborhoods[k]))
            k = 0;
        else
            ++k;
    }
    return cost;
}

long long VariableNeighborhoodDescent::rvnd(Schedule &s)
{
    load(s);
    const auto all = availableNeighborhoods();
    auto list = all;

    while (!list.empty())
    {
        std::uniform_int_distribution<std::size_t> dist(0, list.size() - 1);
        const std::size_t idx = dist(gen);
        if (explore(list[idx]))
            list = all;
        else
            list.erase(list.begin() + idx);
    }
    return cost;
}

void VariableNeighborhoodDescent::insertCheapest(Schedule &s, const std::vector<int> &flights)
{
    load(s);
    for (int f : flights)
    {
        const Piece piece{&f, 1};
        long long bestIncrease = INF;
        int bestR = 0, bestP = 0;

        for (int r = 0; r < static_cast<int>(s.size()); ++r)
        {
            const long long total = states[r].total;
            const int len = static_cast<int>(s[r].size());
            for (int p = 0; p <= len; ++p)
            {
                const long long limit = (bestIncrease == INF) ? INF : total + bestIncrease;
                if (prefixCost(r, p) >= limit)
                    break;
                const long long c = evaluate(r, p, &piece, 1, p, limit);
                if (c < limit)
                {
                    bestIncrease = c - total;
                    bestR = r, bestP = p;
                }
            }
        }

        s[bestR].insert(s[bestR].begin() + bestP, f);
        rebuild(bestR);
        cost += bestIncrease;
    }
}
