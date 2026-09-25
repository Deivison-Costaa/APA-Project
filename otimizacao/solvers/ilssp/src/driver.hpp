// ILS-SP driver: rounds of k parallel ILS trajectories (one per thread),
// followed by a set-partitioning MIP over the merged column pool.
#pragma once

#include <atomic>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "options.hpp"
#include "spsolver.hpp"

struct SpStats {
    int solves = 0;
    int improved = 0;       // SP beat the best ILS solution of that round
    double totalTime = 0.0;
    double maxTime = 0.0;
    size_t maxCols = 0;
};

class Driver {
public:
    Driver(const Instance& ins, const Options& opt);
    ~Driver();

    // Runs until the deadline (steady-clock seconds); returns the best solution.
    Solution solve(Solution start, double tStart, double deadline, const std::string& outFile);

private:
    struct Worker;
    const Instance& ins_;
    const Options& opt_;
    std::vector<std::unique_ptr<Worker>> workers_;
    ColumnPool pool_;
    std::set<std::string> ingested_;
    SpStats sp_;
    std::atomic<bool> stop_{false};  // --target reached

    int ingest(Solution& best);
    void mergePools(long long round, const Solution& best);
    bool runSp(Solution& best, double deadline, long long round, const std::string& tag);
};
