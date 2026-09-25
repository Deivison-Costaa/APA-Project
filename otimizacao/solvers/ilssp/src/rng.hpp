// Small fast PRNG (xoshiro256**), seeded with splitmix64.
#pragma once

#include <cstdint>

class Rng {
public:
    explicit Rng(uint64_t seed = 1) { reseed(seed); }

    void reseed(uint64_t seed) {
        uint64_t z = seed;
        for (auto& v : s_) v = splitmix(z);
    }

    uint64_t next() {
        const uint64_t result = rotl(s_[1] * 5, 7) * 9;
        const uint64_t t = s_[1] << 17;
        s_[2] ^= s_[0];
        s_[3] ^= s_[1];
        s_[1] ^= s_[2];
        s_[0] ^= s_[3];
        s_[2] ^= t;
        s_[3] = rotl(s_[3], 45);
        return result;
    }

    // Uniform integer in [0, n).
    int below(int n) { return static_cast<int>((next() >> 33) % static_cast<uint64_t>(n)); }
    // Uniform integer in [lo, hi].
    int range(int lo, int hi) { return lo + below(hi - lo + 1); }
    // Uniform double in [0, 1).
    double uniform() { return static_cast<double>(next() >> 11) * 0x1.0p-53; }

private:
    uint64_t s_[4];

    static uint64_t rotl(uint64_t x, int k) { return (x << k) | (x >> (64 - k)); }
    static uint64_t splitmix(uint64_t& x) {
        uint64_t z = (x += 0x9e3779b97f4a7c15ULL);
        z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
        z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
        return z ^ (z >> 31);
    }
};
