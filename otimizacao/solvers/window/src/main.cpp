// window: time-window decomposition matheuristic + granular LS for P|r_j,s_ij|sum p_j(S_j-r_j)
#include <cstdio>
#include <cstdlib>
#include <stdexcept>
#include <string>

#include "driver.hpp"
#include "selftest.hpp"

namespace {
void usage() {
    std::printf(
        "usage: window <instance> [--time s] [--seed k] [--threads k] [--init sol] [--out file|dir]\n"
        "              [--pool dir] [--checker check.py] [--mode hybrid|ils|window|selftest]\n"
        "              [--wsize flights] [--wruns k] [--wnodes n] [--quiet]\n");
}
}  // namespace

int main(int argc, char** argv) {
    if (argc < 2) {
        usage();
        return 2;
    }
    Options o;
    o.instPath = argv[1];
    try {
        for (int a = 2; a < argc; ++a) {
            const std::string k = argv[a];
            auto val = [&]() -> std::string {
                if (a + 1 >= argc) throw std::runtime_error("missing value for " + k);
                return argv[++a];
            };
            if (k == "--time") o.timeLimit = std::stod(val());
            else if (k == "--seed") o.seed = std::stoull(val());
            else if (k == "--threads") o.threads = std::stoi(val());
            else if (k == "--init") o.initPath = val();
            else if (k == "--out") o.outPath = val();
            else if (k == "--pool") o.poolDir = val();
            else if (k == "--checker") o.checker = val();
            else if (k == "--mode") o.mode = val();
            else if (k == "--wsize") o.wSize = std::stoi(val());
            else if (k == "--wruns") o.wRunways = std::stoi(val());
            else if (k == "--wnodes") o.wNodes = std::stoll(val());
            else if (k == "--quiet") o.verbose = 0;
            else if (k == "--verbose") o.verbose = std::stoi(val());
            else if (k == "--K") o.nbK = std::stoi(val());
            else if (k == "--W") o.nbW = std::stoi(val());
            else if (k == "--kmin") o.kMin = std::stoi(val());
            else if (k == "--kmax") o.kMax = std::stoi(val());
            else if (k == "--t0") o.temp0 = std::stod(val());
            else if (k == "--t1") o.temp1 = std::stod(val());
            else if (k == "--wfrac") o.windowFrac = std::stod(val());
            else if (k == "--focus") o.focus = std::stod(val());
            else if (k == "--epochs") o.epochs = std::stoi(val());
            else if (k == "--archive") o.archive = std::stoi(val());
            else if (k == "--wt0") o.wt0 = std::stoi(val());
            else if (k == "--wt1") o.wt1 = std::stoi(val());
            else if (k == "--wtl") o.wtl = std::stod(val());
            else if (k == "--dcap") o.dcap = std::stoi(val());
            else if (k == "--oropt") o.orOpt = std::stoi(val());
            else throw std::runtime_error("unknown option " + k);
        }
        if (o.threads < 1) o.threads = 1;
        const Instance I = Instance::read(o.instPath);
        if (o.mode == "selftest") return runSelfTest(I, o.seed);
        return runDriver(I, o);
    } catch (const std::exception& e) {
        std::fprintf(stderr, "error: %s\n", e.what());
        usage();
        return 2;
    }
}
