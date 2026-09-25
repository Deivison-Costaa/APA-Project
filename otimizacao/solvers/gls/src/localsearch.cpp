#include "localsearch.hpp"

#include <algorithm>
#include <cstring>

LocalSearch::LocalSearch(State& st, const Neighbors& nb, const LSParams& prm)
    : st_(st), nb_(nb), prm_(prm) {
    prm_.maxSeg = std::clamp(prm_.maxSeg, 1, 7);
    prm_.maxCross = std::clamp(prm_.maxCross, 0, 7);
}

long LocalSearch::run() {
    long cnt = 0;
    auto& dq = st_.dirty;
    size_t head = 0;
    while (head < dq.size()) {
        const int f = dq[head++];
        st_.isDirty[f] = 0;
        if (improveFlight(f)) ++cnt;
        if (head > 4096 && head * 2 > dq.size()) {
            dq.erase(dq.begin(), dq.begin() + static_cast<long>(head));
            head = 0;
        }
    }
    dq.clear();
    return cnt;
}

namespace {
void setPiece(Piece& pc, int t, int X, int a, const int* E, int ne, int Y, int b) {
    pc.target = t;
    pc.X = X;
    pc.a = a;
    pc.Y = Y;
    pc.b = b;
    pc.ne = ne;
    if (ne) std::memcpy(pc.E, E, sizeof(int) * ne);
}
}  // namespace

void LocalSearch::record2(ll delta, int tA, int XA, int aA, const int* EA, int neA, int YA, int bA,
                          int tB, int XB, int aB, const int* EB, int neB, int YB, int bB) {
    if (delta >= best_.delta) return;
    bestType_ = curType_;
    best_.delta = delta;
    best_.np = 2;
    setPiece(best_.part[0], tA, XA, aA, EA, neA, YA, bA);
    setPiece(best_.part[1], tB, XB, aB, EB, neB, YB, bB);
}

void LocalSearch::record1(ll delta, int t, int X, int a, const int* E, int ne, int Y, int b) {
    if (delta >= best_.delta) return;
    bestType_ = curType_;
    best_.delta = delta;
    best_.np = 1;
    setPiece(best_.part[0], t, X, a, E, ne, Y, b);
}

bool LocalSearch::improveFlight(int f) {
    const int A = st_.where[f], i = st_.pos[f];
    const int lenA = st_.rw[A].len();
    best_.delta = 0;
    best_.np = 0;
    for (int L = 1; L <= prm_.maxSeg; ++L) {
        remF_[L] = (i + L <= lenA) ? st_.eval(A, i - 1, nullptr, 0, A, i + L, INF_COST) : INF_COST;
        remB_[L] = (i - L + 1 >= 0) ? st_.eval(A, i - L, nullptr, 0, A, i + 1, INF_COST) : INF_COST;
    }
    evals += 2 * prm_.maxSeg;
    const auto& P = nb_.pred[f];
    const int kp = std::min<int>(prm_.kPred, static_cast<int>(P.size()));
    for (int x = 0; x < kp && !done(); ++x) {
        const int g = P[x];
        if (st_.where[g] != A) interPred(f, A, i, g);
        else intraPred(f, A, i, st_.pos[g]);
    }
    const auto& Sx = nb_.succ[f];
    const int ks = std::min<int>(prm_.kPred, static_cast<int>(Sx.size()));
    for (int x = 0; x < ks && !done(); ++x) {
        const int h = Sx[x];
        if (st_.where[h] != A) interSucc(f, A, i, h);
        else intraSucc(f, A, i, st_.pos[h]);
    }
    if (!done()) runwayStarts(f, A, i);
    ++flightsExamined;
    if (best_.delta < 0) {
        st_.apply(best_);
        ++appliedBy[bestType_];
        ++applied;
        return true;
    }
    return false;
}

void LocalSearch::interPred(int f, int A, int i, int g) {
    const int B = st_.where[g], q = st_.pos[g];
    const Runway& RA = st_.rw[A];
    const Runway& RB = st_.rw[B];
    const int lenA = RA.len(), lenB = RB.len();
    const ll base = RA.cost() + RB.cost();
    if (prm_.prune) {
        // every move here keeps A[0..i-1] and B[0..q] and puts f right after g
        const int rf = st_.in.r[f];
        const ll dl = static_cast<ll>(st_.in.p[f]) * std::max(0, RB.S[q] + st_.in.gap(g, f) - rf);
        if (dl + RA.cum[i] + RB.cum[q + 1] - base >= best_.delta) { ++pruned; return; }
    }
    const int* sa = RA.seq.data();
    const int* sb = RB.seq.data();
    curType_ = 0;
    for (int L = 1; L <= prm_.maxSeg && i + L <= lenA; ++L) {
        const ll nb = st_.eval(B, q, sa + i, L, B, q + 1, base + best_.delta - 1 - remF_[L]);
        ++evals, ++evalsBy[curType_];
        if (nb < INF_COST)
            record2(remF_[L] + nb - base, A, A, i - 1, nullptr, 0, A, i + L, B, B, q, sa + i, L, B, q + 1);
    }
    curType_ = 1;
    for (int L1 = 1; L1 <= prm_.maxCross && i + L1 <= lenA; ++L1) {
        for (int L2 = 1; L2 <= prm_.maxCross && q + 1 + L2 <= lenB; ++L2) {
            const ll nb = st_.eval(B, q, sa + i, L1, B, q + 1 + L2, base + best_.delta - 1 - RA.cum[i]);
            ++evals, ++evalsBy[curType_];
            if (nb >= INF_COST) continue;
            const ll na = st_.eval(A, i - 1, sb + q + 1, L2, A, i + L1, base + best_.delta - 1 - nb);
            ++evals, ++evalsBy[curType_];
            if (na >= INF_COST) continue;
            record2(na + nb - base, A, A, i - 1, sb + q + 1, L2, A, i + L1, B, B, q, sa + i, L1, B, q + 1 + L2);
        }
    }
    curType_ = 2;
    // 2-opt*: A' = A[0..i-1] + B[q+1..],  B' = B[0..q] + A[i..]
    const ll nb = st_.eval(B, q, nullptr, 0, A, i, base + best_.delta - 1 - RA.cum[i]);
    ++evals, ++evalsBy[curType_];
    if (nb >= INF_COST) return;
    const ll na = st_.eval(A, i - 1, nullptr, 0, B, q + 1, base + best_.delta - 1 - nb);
    ++evals, ++evalsBy[curType_];
    if (na < INF_COST) record2(na + nb - base, A, A, i - 1, nullptr, 0, B, q + 1, B, B, q, nullptr, 0, A, i);
}

void LocalSearch::interSucc(int f, int A, int i, int h) {
    const int B = st_.where[h], q = st_.pos[h];
    const Runway& RA = st_.rw[A];
    const Runway& RB = st_.rw[B];
    const ll base = RA.cost() + RB.cost();
    if (prm_.prune) {
        // every move here puts h right after f (whose start is >= r_f) and keeps
        // A[0..i-maxL] and B[0..q-maxL-1]
        const int maxL = std::max(prm_.maxSeg, prm_.maxCross);
        const int rh = st_.in.r[h];
        const ll dl = static_cast<ll>(st_.in.p[h]) * std::max(0, st_.in.r[f] + st_.in.gap(f, h) - rh);
        if (dl + RA.cum[std::max(0, i - maxL + 1)] + RB.cum[std::max(0, q - maxL)] - base >= best_.delta) {
            ++pruned;
            return;
        }
    }
    const int* sa = RA.seq.data();
    const int* sb = RB.seq.data();
    curType_ = 3;
    for (int L = 1; L <= prm_.maxSeg && i - L + 1 >= 0; ++L) {
        const ll nb = st_.eval(B, q - 1, sa + i - L + 1, L, B, q, base + best_.delta - 1 - remB_[L]);
        ++evals, ++evalsBy[curType_];
        if (nb < INF_COST)
            record2(remB_[L] + nb - base, A, A, i - L, nullptr, 0, A, i + 1, B, B, q - 1, sa + i - L + 1, L, B, q);
    }
    curType_ = 4;
    for (int L1 = 1; L1 <= prm_.maxCross && i - L1 + 1 >= 0; ++L1) {
        for (int L2 = 1; L2 <= prm_.maxCross && q - L2 >= 0; ++L2) {
            const ll nb = st_.eval(B, q - L2 - 1, sa + i - L1 + 1, L1, B, q,
                                   base + best_.delta - 1 - RA.cum[i - L1 + 1]);
            ++evals, ++evalsBy[curType_];
            if (nb >= INF_COST) continue;
            const ll na = st_.eval(A, i - L1, sb + q - L2, L2, A, i + 1, base + best_.delta - 1 - nb);
            ++evals, ++evalsBy[curType_];
            if (na >= INF_COST) continue;
            record2(na + nb - base, A, A, i - L1, sb + q - L2, L2, A, i + 1, B, B, q - L2 - 1, sa + i - L1 + 1,
                    L1, B, q);
        }
    }
    curType_ = 5;
    // 2-opt*: A' = A[0..i] + B[q..],  B' = B[0..q-1] + A[i+1..]
    const ll na = st_.eval(A, i, nullptr, 0, B, q, base + best_.delta - 1 - RB.cum[q]);
    ++evals, ++evalsBy[curType_];
    if (na >= INF_COST) return;
    const ll nb = st_.eval(B, q - 1, nullptr, 0, A, i + 1, base + best_.delta - 1 - na);
    ++evals, ++evalsBy[curType_];
    if (nb < INF_COST) record2(na + nb - base, A, A, i, nullptr, 0, B, q, B, B, q - 1, nullptr, 0, A, i + 1);
}

void LocalSearch::intraPred(int f, int A, int i, int q) {
    curType_ = 6;
    (void)f;
    const Runway& RA = st_.rw[A];
    const int lenA = RA.len();
    const ll costA = RA.cost();
    const int* sa = RA.seq.data();
    for (int L = 1; L <= prm_.maxSeg && i + L <= lenA; ++L) {
        if (q == i - 1 || (q >= i && q < i + L)) break;
        if (q < i - 1) {  // A[0..q] + seg + A[q+1..i-1] + A[i+L..]
            const int mid = i - 1 - q, ne = L + mid;
            if (ne > MAXE) break;
            std::memcpy(tmp_, sa + i, sizeof(int) * L);
            std::memcpy(tmp_ + L, sa + q + 1, sizeof(int) * mid);
            const ll na = st_.eval(A, q, tmp_, ne, A, i + L, costA + best_.delta - 1);
            ++evals, ++evalsBy[curType_];
            if (na < INF_COST) record1(na - costA, A, A, q, tmp_, ne, A, i + L);
        } else {  // A[0..i-1] + A[i+L..q] + seg + A[q+1..]
            const int mid = q - i - L + 1, ne = mid + L;
            if (ne > MAXE) break;
            std::memcpy(tmp_, sa + i + L, sizeof(int) * mid);
            std::memcpy(tmp_ + mid, sa + i, sizeof(int) * L);
            const ll na = st_.eval(A, i - 1, tmp_, ne, A, q + 1, costA + best_.delta - 1);
            ++evals, ++evalsBy[curType_];
            if (na < INF_COST) record1(na - costA, A, A, i - 1, tmp_, ne, A, q + 1);
        }
    }
    const int j = q + 1;  // swap f with succ(g)
    if (j < lenA && j != i) {
        const int lo = std::min(i, j), hi = std::max(i, j), ne = hi - lo + 1;
        if (ne <= MAXE) {
            std::memcpy(tmp_, sa + lo, sizeof(int) * ne);
            std::swap(tmp_[0], tmp_[ne - 1]);
            const ll na = st_.eval(A, lo - 1, tmp_, ne, A, hi + 1, costA + best_.delta - 1);
            ++evals, ++evalsBy[curType_];
            if (na < INF_COST) record1(na - costA, A, A, lo - 1, tmp_, ne, A, hi + 1);
        }
    }
}

void LocalSearch::intraSucc(int f, int A, int i, int q) {
    curType_ = 6;
    (void)f;
    const Runway& RA = st_.rw[A];
    const ll costA = RA.cost();
    const int* sa = RA.seq.data();
    for (int L = 1; L <= prm_.maxSeg && i - L + 1 >= 0; ++L) {
        const int s0 = i - L + 1;
        if (q == i + 1 || (q >= s0 && q <= i)) break;
        if (q > i + 1) {  // A[0..s0-1] + A[i+1..q-1] + seg + A[q..]
            const int mid = q - 1 - i, ne = mid + L;
            if (ne > MAXE) break;
            std::memcpy(tmp_, sa + i + 1, sizeof(int) * mid);
            std::memcpy(tmp_ + mid, sa + s0, sizeof(int) * L);
            const ll na = st_.eval(A, s0 - 1, tmp_, ne, A, q, costA + best_.delta - 1);
            ++evals, ++evalsBy[curType_];
            if (na < INF_COST) record1(na - costA, A, A, s0 - 1, tmp_, ne, A, q);
        } else {  // q < s0: A[0..q-1] + seg + A[q..s0-1] + A[i+1..]
            const int mid = s0 - q, ne = L + mid;
            if (ne > MAXE) break;
            std::memcpy(tmp_, sa + s0, sizeof(int) * L);
            std::memcpy(tmp_ + L, sa + q, sizeof(int) * mid);
            const ll na = st_.eval(A, q - 1, tmp_, ne, A, i + 1, costA + best_.delta - 1);
            ++evals, ++evalsBy[curType_];
            if (na < INF_COST) record1(na - costA, A, A, q - 1, tmp_, ne, A, i + 1);
        }
    }
    const int j = q - 1;  // swap f with pred(h)
    if (j >= 0 && j != i) {
        const int lo = std::min(i, j), hi = std::max(i, j), ne = hi - lo + 1;
        if (ne <= MAXE) {
            std::memcpy(tmp_, sa + lo, sizeof(int) * ne);
            std::swap(tmp_[0], tmp_[ne - 1]);
            const ll na = st_.eval(A, lo - 1, tmp_, ne, A, hi + 1, costA + best_.delta - 1);
            ++evals, ++evalsBy[curType_];
            if (na < INF_COST) record1(na - costA, A, A, lo - 1, tmp_, ne, A, hi + 1);
        }
    }
}

void LocalSearch::runwayStarts(int f, int A, int i) {
    curType_ = 7;
    const Runway& RA = st_.rw[A];
    const int lenA = RA.len();
    const int* sa = RA.seq.data();
    const int rf = st_.in.r[f];
    for (int B = 0; B < st_.in.m && !done(); ++B) {
        if (B == A) continue;
        const Runway& RB = st_.rw[B];
        if (RB.len() > 0 && RB.S[0] < rf) continue;
        const ll base = RA.cost() + RB.cost();
        for (int L = 1; L <= prm_.maxSeg && i + L <= lenA; ++L) {
            const ll nb = st_.eval(B, -1, sa + i, L, B, 0, base + best_.delta - 1 - remF_[L]);
            ++evals, ++evalsBy[curType_];
            if (nb < INF_COST)
                record2(remF_[L] + nb - base, A, A, i - 1, nullptr, 0, A, i + L, B, -1, -1, sa + i, L, B, 0);
        }
        // A' = A[0..i-1] + B,  B' = A[i..]
        const ll nb = st_.eval(-1, -1, nullptr, 0, A, i, base + best_.delta - 1 - RA.cum[i]);
        ++evals, ++evalsBy[curType_];
        if (nb >= INF_COST) continue;
        const ll na = st_.eval(A, i - 1, nullptr, 0, B, 0, base + best_.delta - 1 - nb);
        ++evals, ++evalsBy[curType_];
        if (na < INF_COST) record2(na + nb - base, A, A, i - 1, nullptr, 0, B, 0, B, -1, -1, nullptr, 0, A, i);
    }
}
