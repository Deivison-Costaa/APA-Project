#include "Instance.hpp"
#include "GreedyAlgorithm.hpp"
#include "VariableNeighborhoodDescent.hpp"
#include "MetaHeuristics.hpp"

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <limits>
#include <map>
#include <sstream>
#include <string>
#include <vector>

using namespace std;

namespace
{
    // Valores ótimos (opt) ou limitantes inferiores (LB) fornecidos para as instâncias
    const map<string, long long> bestKnown = {
        {"n3m10A", 7483}, {"n3m10B", 1277}, {"n3m10C", 2088}, {"n3m10D", 322}, {"n3m10E", 3343},
        {"n3m20A", 8230}, {"n3m20B", 1820}, {"n3m20C", 855}, {"n3m20D", 4357}, {"n3m20E", 3798},
        {"n3m40A", 112}, {"n3m40B", 880}, {"n3m40C", 1962}, {"n3m40D", 263}, {"n3m40E", 1192},
        {"n5m50A", 0}, {"n5m50B", 0}, {"n5m50C", 0}, {"n5m50D", 0}, {"n5m50E", 0}};

    struct Options
    {
        string instancePath;
        string algorithm = "ils";
        string outputDir = "results";
        string initialSolution;
        string checkSolution;
        bool bench = false;
        string benchDir = "Instances";
        int runs = 10;
        IlsParameters ils;
        LnsParameters lns;
    };

    void usage(const char *prog)
    {
        cout << "Uso:\n"
             << "  " << prog << " <instancia> [opcoes]\n"
             << "  " << prog << " --bench [--dir Instances] [--runs 10] [opcoes]\n\n"
             << "Opcoes:\n"
             << "  --algo greedy|grasp|vnd|rvnd|ils|lns   algoritmo (padrao: ils)\n"
             << "  --out DIR          pasta dos arquivos de saida (padrao: results)\n"
             << "  --seed S           semente (0: aleatoria)\n"
             << "  --time T           limite de tempo em segundos para ILS/LNS\n"
             << "  --threads T        threads do ILS (padrao: todas)\n"
             << "  --restarts N       reinicios do ILS (padrao: 16)\n"
             << "  --ils-iter N       iteracoes sem melhora por reinicio do ILS\n"
             << "  --lns-iter N       iteracoes do LNS (padrao: 1000)\n"
             << "  --init ARQ         solucao inicial para ILS/LNS\n"
             << "  --check ARQ        apenas valida e calcula o custo de uma solucao\n"
             << "  -v                 imprime o progresso da meta-heuristica\n";
    }

    bool parseArgs(int argc, char **argv, Options &opt)
    {
        bool lnsIterGiven = false;
        for (int i = 1; i < argc; ++i)
        {
            string a = argv[i];
            auto next = [&]() -> string
            {
                if (i + 1 >= argc)
                    throw invalid_argument("faltou o valor de " + a);
                return argv[++i];
            };

            if (a == "-h" || a == "--help")
                return false;
            else if (a == "--bench")
                opt.bench = true;
            else if (a == "--dir")
                opt.benchDir = next();
            else if (a == "--runs")
                opt.runs = stoi(next());
            else if (a == "--algo")
                opt.algorithm = next();
            else if (a == "--out")
                opt.outputDir = next();
            else if (a == "--seed")
                opt.ils.seed = opt.lns.seed = static_cast<unsigned>(stoul(next()));
            else if (a == "--time")
                opt.ils.timeLimit = opt.lns.timeLimit = stod(next());
            else if (a == "--threads")
                opt.ils.threads = stoi(next());
            else if (a == "--restarts")
                opt.ils.maxIter = stoi(next());
            else if (a == "--ils-iter")
                opt.ils.maxIterIls = stoi(next());
            else if (a == "--lns-iter")
            {
                opt.lns.maxIterations = stoi(next());
                lnsIterGiven = true;
            }
            else if (a == "--init")
                opt.initialSolution = next();
            else if (a == "--check")
                opt.checkSolution = next();
            else if (a == "-v")
                opt.ils.verbose = opt.lns.verbose = true;
            else if (!a.empty() && a[0] != '-' && opt.instancePath.empty())
                opt.instancePath = a;
            else
                throw invalid_argument("opcao desconhecida: " + a);
        }
        // Com limite de tempo e sem --lns-iter, o LNS roda até o tempo acabar
        if (opt.lns.timeLimit > 0 && !lnsIterGiven)
            opt.lns.maxIterations = numeric_limits<int>::max();
        return opt.bench || !opt.instancePath.empty();
    }

    template <typename F>
    double timeIt(F &&f)
    {
        auto start = chrono::steady_clock::now();
        f();
        return chrono::duration<double>(chrono::steady_clock::now() - start).count();
    }

    // Executa um único algoritmo; `initial` (opcional) é o ponto de partida de VND/ILS/LNS
    Schedule solve(const Instance &instance, const Options &opt, const string &algorithm,
                   const Schedule &initial, unsigned seed)
    {
        GreedyAlgorithm greedy;
        std::mt19937 gen(seed != 0 ? seed : std::random_device{}());

        if (algorithm == "greedy")
            return greedy.nearestNeighbor(instance);
        if (algorithm == "grasp")
            return greedy.graspNearestNeighbor(instance, 0.2, gen);
        if (algorithm == "vnd" || algorithm == "rvnd")
        {
            Schedule s = initial.empty() ? greedy.nearestNeighbor(instance) : initial;
            VariableNeighborhoodDescent localSearch(instance, gen());
            if (algorithm == "vnd")
                localSearch.vnd(s);
            else
                localSearch.rvnd(s);
            return s;
        }

        MetaHeuristics meta(instance);
        if (algorithm == "ils")
        {
            IlsParameters p = opt.ils;
            p.seed = seed;
            return meta.ils(p, initial);
        }
        if (algorithm == "lns")
        {
            LnsParameters p = opt.lns;
            p.seed = seed;
            return meta.lns(p, initial);
        }
        throw invalid_argument("algoritmo desconhecido: " + algorithm);
    }

    int runSingle(const Options &opt)
    {
        Instance instance;
        if (!instance.read(opt.instancePath))
            return 1;

        if (!opt.checkSolution.empty())
        {
            Schedule s = instance.readSolution(opt.checkSolution);
            if (s.empty())
                return 1;
            cout << "Solucao valida. Custo = " << instance.calculateTotalCost(s) << "\n";
            return 0;
        }

        Schedule initial;
        if (!opt.initialSolution.empty())
        {
            initial = instance.readSolution(opt.initialSolution);
            if (initial.empty())
                return 1;
        }

        Schedule solution;
        double elapsed = timeIt([&]
                                { solution = solve(instance, opt, opt.algorithm, initial, opt.ils.seed); });

        string reason;
        if (!instance.isFeasible(solution, &reason))
        {
            cerr << "ERRO: solucao inviavel (" << reason << ")\n";
            return 1;
        }

        const long long cost = instance.calculateTotalCost(solution);
        cout << "Instancia: " << opt.instancePath << " | Algoritmo: " << opt.algorithm
             << " | Custo: " << cost << " | Tempo: " << fixed << setprecision(6) << elapsed << " s\n";

        string path = instance.writeSolution(opt.outputDir, opt.instancePath, solution);
        if (!path.empty())
            cout << "Solucao gravada em: " << path << "\n";
        return 0;
    }

    struct Stats
    {
        long long best = numeric_limits<long long>::max();
        double sumCost = 0, sumTime = 0;
        int runs = 0;
        Schedule bestSolution;

        void add(long long cost, double time, const Schedule &s)
        {
            sumCost += cost;
            sumTime += time;
            ++runs;
            if (cost < best)
            {
                best = cost;
                bestSolution = s;
            }
        }
        double avgCost() const { return sumCost / runs; }
        double avgTime() const { return sumTime / runs; }
    };

    string gapStr(double value, long long reference)
    {
        if (reference <= 0)
            return "-";
        ostringstream ss;
        ss << fixed << setprecision(2) << (value - reference) * 100.0 / reference;
        return ss.str();
    }

    // Gera a tabela de resultados pedida no enunciado para todas as instâncias da pasta
    int runBench(const Options &opt)
    {
        namespace fs = std::filesystem;
        vector<string> files;
        for (const auto &entry : fs::directory_iterator(opt.benchDir))
        {
            const string stem = entry.path().stem().string();
            if (entry.is_regular_file() && entry.path().extension() == ".txt" && stem.find('_') == string::npos)
                files.push_back(entry.path().string());
        }
        sort(files.begin(), files.end());
        if (files.empty())
        {
            cerr << "Nenhuma instancia encontrada em " << opt.benchDir << "\n";
            return 1;
        }

        const vector<string> algorithms = {"greedy", "vnd", "ils"};
        ostringstream table;
        table << "| instancia | otimo |";
        for (const auto &a : algorithms)
            table << " " << a << " media | " << a << " melhor | " << a << " tempo (s) | " << a << " gap (%) |";
        table << "\n|---|---|";
        for (size_t i = 0; i < algorithms.size(); ++i)
            table << "---|---|---|---|";
        table << "\n";

        for (const auto &file : files)
        {
            Instance instance;
            if (!instance.read(file))
                continue;

            const string name = fs::path(file).stem().string();
            auto it = bestKnown.find(name);
            const long long reference = (it != bestKnown.end()) ? it->second : -1;

            table << "| " << name << " | " << (reference >= 0 ? to_string(reference) : "-") << " |";
            cout << name << ":" << flush;

            for (const auto &algorithm : algorithms)
            {
                Stats stats;
                for (int run = 0; run < opt.runs; ++run)
                {
                    const unsigned seed = opt.ils.seed != 0 ? opt.ils.seed + run : 0;
                    Schedule s;
                    double t = timeIt([&]
                                      { s = solve(instance, opt, algorithm, {}, seed); });
                    if (!instance.isFeasible(s))
                    {
                        cerr << "\nERRO: " << algorithm << " gerou solucao inviavel em " << name << "\n";
                        return 1;
                    }
                    stats.add(instance.calculateTotalCost(s), t, s);
                }

                instance.writeSolution(opt.outputDir + "/" + algorithm, name, stats.bestSolution);
                table << fixed << setprecision(1) << " " << stats.avgCost() << " | " << stats.best << " | "
                      << setprecision(6) << stats.avgTime() << " | " << gapStr(stats.avgCost(), reference) << " |";
                cout << " " << algorithm << "=" << stats.best << flush;
            }
            table << "\n";
            cout << "\n";
        }

        cout << "\n"
             << table.str();
        fs::create_directories(opt.outputDir);
        ofstream(opt.outputDir + "/tabela.md") << table.str();
        cout << "\nTabela gravada em " << opt.outputDir << "/tabela.md\n";
        return 0;
    }
}

int main(int argc, char **argv)
{
    Options opt;
    try
    {
        if (!parseArgs(argc, argv, opt))
        {
            usage(argv[0]);
            return 1;
        }
        return opt.bench ? runBench(opt) : runSingle(opt);
    }
    catch (const exception &e)
    {
        cerr << "Erro: " << e.what() << "\n";
        usage(argv[0]);
        return 1;
    }
}
