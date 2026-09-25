#pragma once
#include <cstdint>
#include <string>

#include "instance.hpp"

struct Options {
    std::string instPath, initPath, outPath, poolDir, checker, mode = "hybrid";
    double timeLimit = 60;
    uint64_t seed = 1;
    int threads = 1;
    int verbose = 1;
    int nbK = 40, nbW = 300;
    int kMin = 4, kMax = 14;
    double temp0 = 0, temp1 = 0;
    double focus = 0.0;
    int epochs = 3;
    int archive = 8;
    int wSize = 30, wRunways = 0;
    long long wNodes = 200000;
    double windowFrac = 0.08;
    int wt0 = 0, wt1 = 0;
    double wtl = 2.0;
    int dcap = 400;
    int orOpt = 0;
};

int runDriver(const Instance& I, const Options& o);
