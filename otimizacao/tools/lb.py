"""Assignment lower bound for P | r_j, s_ij | sum p_j (S_j - r_j).

Every flight j gets one predecessor: a flight i (cost p_j * max(0, r_i + c_i + t_ij - r_j),
valid because S_i >= r_i) or one of m runway starts (cost 0). Each flight/start is the
predecessor of at most one flight. The optimum of this assignment is a lower bound.
Also reports the minimum number of zero-delay chains (min path cover of the DAG
i -> j iff r_i + c_i + t_ij <= r_j).
"""
import sys
import numpy as np
from scipy.optimize import linear_sum_assignment
from scipy.sparse import csr_matrix
from scipy.sparse.csgraph import maximum_bipartite_matching


def read(path):
    tok = open(path).read().split()
    n, m = int(tok[0]), int(tok[1])
    v = np.array(tok[2:], dtype=np.int64)
    r, c, p = v[:n], v[n:2 * n], v[2 * n:3 * n]
    t = v[3 * n:3 * n + n * n].reshape(n, n)
    return n, m, r, c, p, t


def main(path):
    n, m, r, c, p, t = read(path)
    # delay[i, j] = max(0, r_i + c_i + t_ij - r_j)  (i predecessor of j)
    delay = np.maximum(0, (r + c)[:, None] + t - r[None, :])
    arc = delay * p[None, :]
    big = 10 ** 12
    np.fill_diagonal(arc, big)
    # rows = successor j, cols = predecessor i (n flights) + m starts
    cost = np.zeros((n, n + m), dtype=np.int64)
    cost[:, :n] = arc.T
    rows, cols = linear_sum_assignment(cost)
    lb = int(cost[rows, cols].sum())
    starts = int((cols >= n).sum())
    zero_arcs = (delay == 0)
    np.fill_diagonal(zero_arcs, False)
    match = maximum_bipartite_matching(csr_matrix(zero_arcs.astype(np.int8)), perm_type='column')
    cover = n - int((match >= 0).sum())
    print(f"{path}: n={n} m={m} assignmentLB={lb} startsUsed={starts} minZeroDelayChains={cover}")


if __name__ == "__main__":
    for f in sys.argv[1:]:
        main(f)
