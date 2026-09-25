#ifndef GREEDYALGORITHM_HPP
#define GREEDYALGORITHM_HPP

#include "Instance.hpp"
#include <random>

class GreedyAlgorithm
{
public:
    // Voos em ordem de liberação; cada um vai para a pista que gera a menor multa
    Schedule nearestNeighbor(const Instance &instance) const;

    // Versão GRASP: escolhe aleatoriamente entre as pistas cuja multa está em
    // [min, min + alpha * (max - min)] (lista restrita de candidatos)
    Schedule graspNearestNeighbor(const Instance &instance, double alpha, std::mt19937 &gen) const;
};

#endif
