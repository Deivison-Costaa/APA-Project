// Individuals and the HGS population (biased fitness = cost rank + diversity rank).
#pragma once
#include <memory>
#include <vector>

#include "instance.hpp"
#include "params.hpp"

struct Individual {
    Routes routes;
    std::vector<int> S;     // start time of every flight
    std::vector<int> succ;  // successor on the runway, -1 = last
    std::vector<int> pred;  // predecessor on the runway, -1 = first
    long long cost = 0;
    double biasedFitness = 0;
    std::vector<std::pair<double, Individual*>> proximity;  // ascending distance

    Individual() = default;
    Individual(const Instance& ins, Routes rts);  // evaluates routes
    double avgClosest(int k) const;
};

// Broken-pairs distance on successor arcs (runway starts count as arcs from a dummy).
double brokenPairsDistance(const Individual& a, const Individual& b);
// Fraction of flights whose start time differs (identical start vectors = same cost).
double startsDistance(const Individual& a, const Individual& b);

class Population {
public:
    Population(const Instance& ins, const Params& par) : I(ins), P(par) {}

    // Adds a copy; runs survivor selection when the population exceeds mu + lambda.
    void add(const Individual& ind);
    const Individual& tournament(Rng& rng);
    const Individual* best() const;
    void clear() { pop_.clear(); }
    int size() const { return (int)pop_.size(); }
    double averageCost() const;
    double averageDiversity() const;

private:
    const Instance& I;
    const Params& P;
    std::vector<std::unique_ptr<Individual>> pop_;

    void updateBiasedFitness();
    void removeWorst();
};
