// Greedy list-scheduling constructions.
#pragma once

#include "rng.hpp"
#include "solution.hpp"

// Flights in release order; each is appended to the runway giving the least
// delay cost, ties broken by the tightest fit (least idle time). With noise>0
// the candidate score is randomly perturbed (for diverse starts).
Solution constructGreedy(const Instance& ins, Rng& rng, double noise);
