// Applies structural edits to a Solution, keeps an undo journal of touched
// runways, and reports the time regions that changed (for LS activation).
#pragma once

#include <vector>

#include "solution.hpp"

constexpr int kMaxList = 8;

struct WindowEdit {
    int k = -1;   // runway
    int lo = 0;   // replace positions [lo, hi) ...
    int hi = 0;
    int L = 0;    // ... by list[0..L)
    int list[kMaxList];
};

struct Region {
    int k;
    int tLo;
    int tHi;
};

class Journal {
public:
    void init(int m);
    void touch(const Solution& s, int k);
    void commit();
    void revert(const Instance& ins, Solution& s);
    const std::vector<int>& touched() const { return touched_; }

private:
    std::vector<int> touched_;
    std::vector<char> isTouched_;
    std::vector<Runway> backup_;
};

class Editor {
public:
    explicit Editor(const Instance& ins) : ins_(ins) {}

    void attach(Solution* s, Journal* j) {
        sol_ = s;
        journal_ = j;
    }

    // Applies 1 or 2 window edits (on distinct runways, or one intra edit).
    // `hint` is a time near the change (used when a runway becomes empty).
    void applyWindows(const WindowEdit* edits, int count, int hint, std::vector<Region>& out);
    // 2-opt*: A keeps [0,ca)+B[cb..], B keeps [0,cb)+A[ca..].
    void applyTwoOpt(int A, int ca, int B, int cb, int hint, std::vector<Region>& out);

    Solution& sol() { return *sol_; }

private:
    const Instance& ins_;
    Solution* sol_ = nullptr;
    Journal* journal_ = nullptr;
    std::vector<int> tmpA_, tmpB_;

    void touch(int k) {
        if (journal_) journal_->touch(*sol_, k);
    }
    Region finish(int k, int lo, int structEnd, int hint);
};
