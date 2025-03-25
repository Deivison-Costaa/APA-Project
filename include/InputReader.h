#ifndef INPUT_READER_H
#define INPUT_READER_H

#include <vector>
#include <string>
#include "Flight.h"

using namespace std;

class InputReader {
public:
    bool read(const string& filename, int& n, int& m, vector<Flight>& flights, vector<vector<int>>& t);
};

#endif