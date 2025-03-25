#include <fstream>
#include <iostream>
#include "InputReader.h"

using namespace std;

bool InputReader::read(const string& filename, int& n, int& m, vector<Flight>& flights, vector<vector<int>>& t) {
    ifstream fin(filename);
    if (!fin) {
        cerr << "Erro ao abrir " << filename << endl;
        return false;
    }
    fin >> n >> m;
    flights.resize(n);
    t.resize(n, vector<int>(n));
    for (int i = 0; i < n; i++) fin >> flights[i].r;
    for (int i = 0; i < n; i++) fin >> flights[i].c;
    for (int i = 0; i < n; i++) fin >> flights[i].p;
    for (int i = 0; i < n; i++)
        for (int j = 0; j < n; j++)
            fin >> t[i][j];
    fin.close();
    return true;
}