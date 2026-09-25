// Stand-in for spsolver.cpp in the HiGHS-free build (ilssp-pure): set partitioning is
// unavailable, so the driver must be run with --no-sp.
#include "../src/spsolver.hpp"

SpResult solveSetPartitioning(const Instance&, const ColumnPool&, const std::vector<int>&,
                              const std::vector<int>&, double, bool) {
    return SpResult{};
}
