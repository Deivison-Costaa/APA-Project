#include "options.hpp"

#include <cstdio>
#include <cstdlib>
#include <stdexcept>

namespace {

[[noreturn]] void usage(const char* msg) {
    if (msg) std::fprintf(stderr, "error: %s\n", msg);
    std::fprintf(stderr,
                 "usage: ilssp <instance> [--time s] [--seed k] [--threads k] [--init file]\n"
                 "             [--out file|dir] [--pool-dir dir] [--no-sp] [--round s]\n"
                 "             [--sp-time s] [--sp-max-cols k] [--T0 x] [--Tf x] [--tol x] [--target cost]\n"
                 "             [--margin t] [--pos-window w] [--max-runways k] [--max-string k]\n"
                 "             [--blink x] [--delayed-seed x] [--insert-window w]\n"
                 "             [--selftest] [--quiet] [--sp-verbose]\n");
    std::exit(2);
}

double toDouble(const std::string& s) {
    try {
        return std::stod(s);
    } catch (...) {
        usage(("bad number: " + s).c_str());
    }
}

long long toInt(const std::string& s) {
    try {
        return std::stoll(s);
    } catch (...) {
        usage(("bad integer: " + s).c_str());
    }
}

}  // namespace

Options parseOptions(int argc, char** argv) {
    Options o;
    for (int i = 1; i < argc; ++i) {
        const std::string a = argv[i];
        auto val = [&]() -> std::string {
            if (i + 1 >= argc) usage(("missing value for " + a).c_str());
            return argv[++i];
        };
        if (a == "--time") o.timeLimit = toDouble(val());
        else if (a == "--seed") o.seed = static_cast<uint64_t>(toInt(val()));
        else if (a == "--threads") o.threads = static_cast<int>(toInt(val()));
        else if (a == "--init") o.initFile = val();
        else if (a == "--out") o.out = val();
        else if (a == "--pool-dir") o.poolDir = val();
        else if (a == "--no-sp") o.useSp = false;
        else if (a == "--target") o.target = toInt(val());
        else if (a == "--round") o.roundTime = toDouble(val());
        else if (a == "--sp-time") o.spTime = toDouble(val());
        else if (a == "--sp-max-cols") o.spMaxCols = static_cast<int>(toInt(val()));
        else if (a == "--T0") o.ils.T0 = toDouble(val());
        else if (a == "--Tf") o.ils.Tf = toDouble(val());
        else if (a == "--tol") o.ils.poolTol = toDouble(val());
        else if (a == "--margin") o.ls.activationMargin = static_cast<int>(toInt(val()));
        else if (a == "--pos-window") o.ls.posWindow = static_cast<int>(toInt(val()));
        else if (a == "--max-runways") o.ruin.maxRunways = static_cast<int>(toInt(val()));
        else if (a == "--max-string") o.ruin.maxString = static_cast<int>(toInt(val()));
        else if (a == "--blink") o.ruin.blink = toDouble(val());
        else if (a == "--delayed-seed") o.ruin.delayedSeed = toDouble(val());
        else if (a == "--insert-window") o.ruin.insertWindow = static_cast<int>(toInt(val()));
        else if (a == "--selftest") o.selfTest = true;
        else if (a == "--quiet") o.quiet = true;
        else if (a == "--sp-verbose") o.spVerbose = true;
        else if (!a.empty() && a[0] == '-') usage(("unknown option " + a).c_str());
        else if (o.instance.empty()) o.instance = a;
        else usage("more than one instance given");
    }
    if (o.instance.empty()) usage("no instance given");
    if (o.timeLimit <= 0 || o.threads < 1 || o.roundTime <= 0 || o.spTime <= 0)
        usage("time/threads/round must be positive");
    if (o.ils.T0 <= 0 || o.ils.Tf <= 0) usage("temperatures must be positive");
    if (o.ruin.maxRunways < 1 || o.ruin.maxString < 1) usage("ruin sizes must be >= 1");
    o.ils.selfTest = o.selfTest;
    o.ls.verify = o.selfTest;
    return o;
}
