#include "Instance.hpp"
#include <iostream>


using namespace std;

int main(void)
{
    string filePath = "Instances/instance0.txt";

    Instance instance;
    instance.read(filePath);
    instance.print();
}