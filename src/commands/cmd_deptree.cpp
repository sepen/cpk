#include "../cpk.h"
#include "../utils.h"
#include "cmd_deptree.h"
#include <algorithm>
#include <set>
#include <string>
#include <vector>

static std::string deptree_pad(int depth) {
    const int n = 1 + 2 * std::max(0, depth);
    return std::string(static_cast<std::size_t>(n), ' ');
}

static void deptree_walk(const std::string& pkg, int depth, std::set<std::string>& expanded) {
    if (expanded.count(pkg)) {
        const std::string mark = is_package_installed(pkg) ? "[i]" : "[ ]";
        print_message(mark + deptree_pad(depth) + pkg + " -->");
        return;
    }

    expanded.insert(pkg);
    const std::string mark = is_package_installed(pkg) ? "[i]" : "[ ]";
    print_message(mark + deptree_pad(depth) + pkg);

    std::vector<std::string> deps;
    if (!get_package_dependency_names(pkg, deps)) {
        return;
    }
    for (const auto& d : deps) {
        deptree_walk(d, depth + 1, expanded);
    }
}

void cmd_deptree(const std::vector<std::string>& args) {
    if (args.empty()) {
        print_message("Package name is required", RED);
        return;
    }

    const std::string& root = args[0];
    std::vector<std::string> probe;
    if (!get_package_dependency_names(root, probe)) {
        return;
    }

    print_message("-- dependencies ([i] = installed, [ ] = not installed, '-->' = seen before)");
    std::set<std::string> expanded;
    deptree_walk(root, 0, expanded);
}
