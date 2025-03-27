#include "Instance.hpp"
#include "GreedyAlgorithm.hpp"
#include <iostream>


using namespace std;

int main(void)
{
    string filePath = "Instances/instance0.txt";

    Instance instance;
    instance.read(filePath);
    instance.print();
    GreedyAlgorithm greedy;
    instance.flightList = greedy.nearestNeighbor(instance);
    cout << "Result: " << endl;

    //preciso colocar isso no Instance depois, estou indo almoçar no momento

    for(unsigned long i = 0; i < instance.flightList.size(); i++)
    {
        cout << "Runway " << i << ": ";
        for(unsigned long j = 0; j < instance.flightList[i].size(); j++)
        {
            cout << instance.flightList[i][j] << " ";
        }
        cout << endl;
    }

    instance.solution = instance.calculateTotalCost(instance.flightList);
    cout << "Total cost: " << instance.solution << "\n\n" << endl;

    instance.print();

    return 0;

}