// Output handling: write best solution to --out and optionally publish to the shared pool
// (validated with the independent checker, atomic rename).
#pragma once
#include <mutex>
#include <string>

#include "solution.hpp"

class Publisher {
public:
    Publisher(std::string outPath, std::string poolDir, std::string checker, std::string instPath,
              std::string instName);
    // Thread-safe. Writes --out if better than anything written so far; publishes to the pool
    // (rate-limited unless force) if better than the pool's best.
    void offer(const Solution& s, bool force = false);
    long long bestWritten() const { return bestWritten_; }

private:
    long long poolBest() const;
    std::string outPath_, poolDir_, checker_, instPath_, instName_;
    long long bestWritten_ = -1, bestPublished_ = -1;
    double lastPublish_ = -1e9;
    std::mutex mu_;
};

std::string instanceName(const std::string& path);
