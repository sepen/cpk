#include "../cpk.h"
#include "../utils.h"
#include "cmd_deptree.h"
#include <iostream>
#include <set>
#include <sstream>
#include <string>
#include <unordered_set>
#include <vector>

static void deptree_format_line(const std::string& mark, int depth, const std::string& pkg,
                                bool cycle, std::ostringstream& out) {
    out << mark << ' ' << std::string(static_cast<std::size_t>(2 * depth), ' ') << pkg;
    if (cycle) {
        out << " -->";
    }
    out << '\n';
}

static void deptree_walk(const std::string& pkg, int depth, std::set<std::string>& expanded,
                         const std::unordered_set<std::string>& installed, std::ostringstream& out) {
    const std::string mark = installed.count(pkg) > 0 ? "[i]" : "[ ]";

    if (expanded.count(pkg)) {
        deptree_format_line(mark, depth, pkg, true, out);
        return;
    }

    expanded.insert(pkg);
    deptree_format_line(mark, depth, pkg, false, out);

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
    tree << "-- dependencies ([i] = installed, '-->' = seen before)\n";
    std::set<std::string> expanded;
    deptree_walk(root, 0, expanded, installed, tree);
    std::cout << tree.str();
}
