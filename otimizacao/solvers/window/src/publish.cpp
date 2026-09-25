#include "publish.hpp"

#include <unistd.h>

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <filesystem>

namespace fs = std::filesystem;

std::string instanceName(const std::string& path) {
    std::string base = fs::path(path).filename().string();
    const auto dot = base.rfind('.');
    return dot == std::string::npos ? base : base.substr(0, dot);
}

Publisher::Publisher(std::string outPath, std::string poolDir, std::string checker, std::string instPath,
                     std::string instName)
    : outPath_(std::move(outPath)), poolDir_(std::move(poolDir)), checker_(std::move(checker)),
      instPath_(std::move(instPath)), instName_(std::move(instName)) {
    if (!outPath_.empty() && fs::is_directory(outPath_))
        outPath_ = (fs::path(outPath_) / (instName_ + "_window_best.txt")).string();
}

long long Publisher::poolBest() const {
    long long best = -1;
    std::error_code ec;
    for (const auto& e : fs::directory_iterator(poolDir_, ec)) {
        const std::string f = e.path().filename().string();
        const std::string pre = instName_ + "_";
        if (f.rfind(pre, 0) != 0 || f.size() < 5 || f.substr(f.size() - 4) != ".txt") continue;
        const std::string rest = f.substr(pre.size());
        char* endp = nullptr;
        const long long v = std::strtoll(rest.c_str(), &endp, 10);
        if (endp == rest.c_str()) continue;
        if (best < 0 || v < best) best = v;
    }
    return best;
}

void Publisher::offer(const Solution& s, bool force) {
    std::lock_guard<std::mutex> lock(mu_);
    const long long c = s.fullCost();
    if (c != s.cost) {
        std::fprintf(stderr, "ERROR: cached cost %lld != full cost %lld; not writing\n", s.cost, c);
        return;
    }
    if (!outPath_.empty() && (bestWritten_ < 0 || c < bestWritten_)) {
        writeSolution(s, outPath_);
        bestWritten_ = c;
    }
    if (poolDir_.empty()) return;
    const double now =
        std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();
    if (!force && now - lastPublish_ < 20.0) return;
    if (bestPublished_ >= 0 && c >= bestPublished_) return;
    const long long pb = poolBest();
    if (pb >= 0 && c >= pb) return;
    lastPublish_ = now;
    const std::string tmp = (fs::path(poolDir_) / (".tmp_window_" + std::to_string(getpid()))).string();
    writeSolution(s, tmp);
    const std::string cmd = "python3 '" + checker_ + "' '" + instPath_ + "' '" + tmp + "' > /dev/null 2>&1";
    if (std::system(cmd.c_str()) != 0) {
        std::fprintf(stderr, "ERROR: checker rejected candidate cost %lld\n", c);
        fs::remove(tmp);
        return;
    }
    const std::string dst =
        (fs::path(poolDir_) / (instName_ + "_" + std::to_string(c) + "_window.txt")).string();
    std::error_code ec;
    fs::rename(tmp, dst, ec);
    if (ec) {
        std::fprintf(stderr, "ERROR: publish rename failed: %s\n", ec.message().c_str());
        return;
    }
    bestPublished_ = c;
    std::printf("PUBLISHED %s\n", dst.c_str());
    std::fflush(stdout);
}
