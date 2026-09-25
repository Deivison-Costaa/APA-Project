// Instance data for P | r_j, s_ij | sum p_j (S_j - r_j).
#pragma once

#include <string>
#include <vector>

struct Instance {
    int n = 0;
    int m = 0;
    std::vector<int> r;          // release time
    std::vector<int> c;          // runway occupation
    std::vector<int> p;          // penalty per time unit of delay
    std::vector<int> t;          // n*n separation matrix, row = from
    std::vector<int> byRelease;  // flights sorted by (r, id)
    std::vector<int> rankOf;     // position of each flight in byRelease
    int maxC = 0;
    int maxT = 0;

    int sep(int from, int to) const { return t[static_cast<size_t>(from) * n + to]; }
};

// Throws std::runtime_error on malformed input.
Instance loadInstance(const std::string& path);
