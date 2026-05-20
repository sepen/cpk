#include "../cpk.h"
#include "../utils.h"
#include "cmd_deptree.h"
#include <algorithm>
#include <set>
#include <sstream>
#include <string>
#include <unordered_set>
#include <vector>

static std::string deptree_pad(int depth) {
    const int n = 1 + 2 * std::max(0, depth);
    return std::string(static_cast<std::size_t>(n), ' ');
}

static void deptree_walk(const std::string& pkg, int depth, std::set<std::string>& expanded,
                         const std::unordered_set<std::string>& installed, std::ostringstream& out) {
    const bool inst = installed.count(pkg) > 0;
    const std::string mark = inst ? "[i]" : "[ ]";

    if (expanded.count(pkg)) {
        out << mark << deptree_pad(depth) << pkg << " -->\n";
        return;
    }

    expanded.insert(pkg);
    out << mark << deptree_pad(depth) << pkg << '\n';

    std::vector<std::string> deps;
    if (!get_package_dependency_names(pkg, deps)) {
        return;
    }
    for (const auto& d : deps) {
        deptree_walk(d, depth + 1, expanded, installed, out);
    }
}

void cmd_deptree(const std::vector<std::string>& args) {
    if (args.empty()) {
        print_message("Package name is required", RED);
        return;
    }

    const std::string& root = args[0];
    cpk_preload_index_deps_cache();

    std::vector<std::string> probe;
    if (!get_package_dependency_names(root, probe)) {
        return;
    }

    std::unordered_set<std::string> installed;
    for (const auto& p : get_installed_packages()) {
        installed.insert(p);
    }

    std::ostringstream tree;
    tree << "-- dependencies ([i] = installed, [ ] = not installed, '-->' = seen before)\n";
    std::set<std::string> expanded;
    deptree_walk(root, 0, expanded, installed, tree);
    print_fmt_lines(tree.str());
}
