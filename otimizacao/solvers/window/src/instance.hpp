// Instance data for P | r_j, s_ij | sum p_j (S_j - r_j)
#pragma once
#include <cstdint>
#include <string>
#include <vector>

struct Instance {
    int n = 0;
    int m = 0;
    std::vector<int> r, c, p;
    std::vector<int> t;       // n*n, row = from
    std::vector<uint8_t> t8;  // compact copy when all t <= 255 (cache-friendlier), else empty

    int T(int i, int j) const {
        const size_t k = static_cast<size_t>(i) * n + j;
        return t8.empty() ? t[k] : static_cast<int>(t8[k]);
    }
    const int* row(int i) const { return t.data() + static_cast<size_t>(i) * n; }

    // Throws std::runtime_error on malformed input.
    static Instance read(const std::string& path);
};
