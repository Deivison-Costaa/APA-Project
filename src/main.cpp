#include "InstanceReader/InstanceReader.hpp"
#include <iostream>


using namespace std;

int main(void)
{
    string filePath = "Include/Instances/instance0.txt";

    InstanceReader instanceReader;
    instanceReader.read(filePath);
    instanceReader.print();
}