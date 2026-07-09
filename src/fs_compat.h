#ifndef FS_COMPAT_H
#define FS_COMPAT_H

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
    auto pit = p.begin(), pend = p.end();
    auto bit = base.begin(), bend = base.end();
    while (pit != pend && bit != bend && *pit == *bit) { ++pit; ++bit; }
    fs::path result;
    for (; bit != bend; ++bit) result /= "..";
    for (; pit != pend; ++pit) result /= *pit;
    return result;
#else
    return fs::relative(p, base);
#endif
}

#endif  // FS_COMPAT_H
