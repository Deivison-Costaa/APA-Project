// Command-line options.
#pragma once

#include <cstdint>
#include <string>

#include "ils.hpp"

struct Options {
    std::string instance;
    std::string initFile;
    std::string out;
    std::string poolDir;     // external solutions to ingest as columns
    double timeLimit = 60.0;
    long long target = -1;     // stop as soon as the best cost is <= target (-1: off)
    uint64_t seed = 1;
    int threads = 1;
    bool useSp = true;
    double roundTime = 30.0;   // ILS seconds between SP solves
    double spTime = 10.0;      // MIP time limit per solve
    int spMaxCols = 5000;     // cap on columns passed to the MIP
    bool selfTest = false;
    bool quiet = false;
    bool spVerbose = false;
    IlsParams ils;
    LsParams ls;
    RuinParams ruin;
};

// Exits with usage on error.
Options parseOptions(int argc, char** argv);
