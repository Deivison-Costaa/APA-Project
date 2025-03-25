#ifndef INPUT_READER_H
#define INPUT_READER_H

#include <vector>
#include <string>
#include "Flight.h"

class InputReader {
public:
    bool read(const std::string& filename, int& n, int& m, std::vector<Flight>& flights, std::vector<std::vector<int>>& t);
};

#endif