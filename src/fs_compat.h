#ifndef FS_COMPAT_H
#define FS_COMPAT_H

#include <vector>

// GCC < 8 has no <filesystem> header at all; it only ships the pre-standard
// Filesystem TS under <experimental/filesystem> / std::experimental::filesystem.
// configure.ac defines CPK_USE_EXPERIMENTAL_FS for those compilers.
#ifdef CPK_USE_EXPERIMENTAL_FS
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
#else
#include <filesystem>
namespace fs = std::filesystem;
#endif

// std::experimental::filesystem (GCC < 8) lacks the free function relative().
// Provide a lexical fallback that matches std::filesystem::relative() for the
// paths this project produces (target always under base).
inline fs::path fs_relative(const fs::path& p, const fs::path& base) {
#ifdef CPK_USE_EXPERIMENTAL_FS
    // Drop empty components: a trailing '/' (e.g. an output dir like "repo/")
    // yields an empty path element that std::filesystem::relative() ignores but
    // a naive component walk would otherwise turn into a spurious "..".
    std::vector<fs::path> pv, bv;
    for (const auto& c : p)    { if (!c.empty()) pv.push_back(c); }
    for (const auto& c : base) { if (!c.empty()) bv.push_back(c); }
    size_t i = 0;
    while (i < pv.size() && i < bv.size() && pv[i] == bv[i]) { ++i; }
    fs::path result;
    for (size_t j = i; j < bv.size(); ++j) { result /= ".."; }
    for (size_t j = i; j < pv.size(); ++j) { result /= pv[j]; }
    return result;
#else
    return fs::relative(p, base);
#endif
}

#endif  // FS_COMPAT_H
