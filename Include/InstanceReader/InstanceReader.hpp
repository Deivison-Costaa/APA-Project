#ifndef INSTANCEREADER_HPP
#define INSTANCEREADER_HPP

#include <vector>
#include <string>

class InstanceReader
{
public:
    int number_of_flights;
    int number_of_runways;

    std::vector<int> r;
    std::vector<int> c;
    std::vector<int> p;
    std::vector<std::vector<int>> t;

    bool read(const std::string &filePath);
    void print() const;
};

#endif
