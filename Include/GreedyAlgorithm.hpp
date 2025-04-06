#ifndef GREEDYALGORITHM_HPP
#define GREEDYALGORITHM_HPP

#include "Instance.hpp"
#include <vector>

// Classe que implementa algoritmos gulosos para o problema de escalonamento de voos
class GreedyAlgorithm{
public:
    // Algoritmo guloso que aloca voos às pistas com base na menor penalidade (vizinho mais próximo)
    std::vector<std::vector<int>> nearestNeighbor(const Instance& instance);

    // Algoritmo guloso (ainda não implementado) baseado em inserção mais barata
    std::vector<std::vector<int>> cheapestInsertion(const Instance& Instance);
};

#endif