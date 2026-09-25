// Common utilities: integer types, RNG, wall-clock timer.
#pragma once
#include <chrono>
#include <cstdint>
#include <limits>

using ll = long long;
constexpr ll INF_COST = std::numeric_limits<ll>::max() / 4;

// xoshiro256** seeded with splitmix64: fast, good quality, reproducible.
class Rng {
public:
    explicit Rng(uint64_t seed = 1) { reseed(seed); }
    void reseed(uint64_t seed) {
        uint64_t z = seed + 0x9E3779B97F4A7C15ULL;
        for (auto& v : s_) {
            z += 0x9E3779B97F4A7C15ULL;
            uint64_t x = z;
            x = (x ^ (x >> 30)) * 0xBF58476D1CE4E5B9ULL;
            x = (x ^ (x >> 27)) * 0x94D049BB133111EBULL;
            v = x ^ (x >> 31);
        }
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
    // uniform integer in [0, n)
    int below(int n) { return static_cast<int>((next() >> 32) * static_cast<uint64_t>(n) >> 32); }
    // uniform integer in [lo, hi]
    int range(int lo, int hi) { return lo + below(hi - lo + 1); }
    // uniform double in [0, 1)
    double uniform() { return (next() >> 11) * (1.0 / 9007199254740992.0); }

private:
    static uint64_t rotl(uint64_t x, int k) { return (x << k) | (x >> (64 - k)); }
    uint64_t s_[4];
};

class Timer {
public:
    Timer() : start_(std::chrono::steady_clock::now()) {}
    double seconds() const {
        return std::chrono::duration<double>(std::chrono::steady_clock::now() - start_).count();
    }

private:
    std::chrono::steady_clock::time_point start_;
};
