// Small fast PRNG (xoshiro256**) seeded by splitmix64.
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
        const uint64_t out = rotl(s_[1] * 5, 7) * 9;
        const uint64_t t = s_[1] << 17;
        s_[2] ^= s_[0];
        s_[3] ^= s_[1];
        s_[1] ^= s_[2];
        s_[0] ^= s_[3];
        s_[2] ^= t;
        s_[3] = rotl(s_[3], 45);
        return out;
    }

    uint32_t next32() { return static_cast<uint32_t>(next() >> 32); }

    // Uniform integer in [0, n).
    int below(int n) { return static_cast<int>((static_cast<uint64_t>(next32()) * static_cast<uint64_t>(n)) >> 32); }

    // Uniform double in (0, 1].
    double u01() { return (static_cast<double>(next() >> 11) + 1.0) * (1.0 / 9007199254740992.0); }

private:
    static uint64_t rotl(uint64_t x, int k) { return (x << k) | (x >> (64 - k)); }
    static uint64_t splitmix(uint64_t& z) {
        uint64_t x = (z += 0x9e3779b97f4a7c15ULL);
        x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ULL;
        x = (x ^ (x >> 27)) * 0x94d049bb133111ebULL;
        return x ^ (x >> 31);
    }
    uint64_t s_[4];
};
