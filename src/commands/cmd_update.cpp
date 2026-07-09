#include "../cpk.h"
#include "../utils.h"
#include "../fs_compat.h"
#include <fstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

static std::string trim_index_line(std::string line) {
    while (!line.empty() && (line.back() == '\r' || line.back() == '\n' || line.back() == ' ' || line.back() == '\t')) {
        line.pop_back();
    }
    size_t i = 0;
    while (i < line.size() && (line[i] == ' ' || line[i] == '\t')) {
        ++i;
    }
    return line.substr(i);
}

// CPKINDEX line "name#ver-rel.arch.cpk: deps" -> unique label without ".cpk"
static std::string index_package_label(const std::string& pkg_line) {
    if (pkg_line.size() >= 4 && pkg_line.compare(pkg_line.size() - 4, 4, ".cpk") == 0) {
        return pkg_line.substr(0, pkg_line.size() - 4);
    }
    return pkg_line;
}

// Port name for CPKINDEX lines "name#version-release.arch.cpk: deps"
static std::string index_pkgname(const std::string& pkg_line) {
    const size_t h = pkg_line.find('#');
    if (h == std::string::npos || h == 0) {
        return pkg_line;
    }
    return pkg_line.substr(0, h);
}

// Map each port name to the label of its newest revision (first CPKINDEX line
// per port name wins). Returns false only if the file cannot be opened.
static bool read_index_labels(const std::string& index_file,
                              std::unordered_map<std::string, std::string>& labels) {
    std::ifstream in(index_file);
    if (!in.is_open()) {
        return false;
    }
    std::string line;
    while (std::getline(in, line)) {
        const std::string trimmed = trim_index_line(line);
        if (!cpk_index_line_valid(trimmed)) {
            continue;
        }
        const std::string pkg_line = cpk_index_line_package(trimmed);
        const std::string pkgname = index_pkgname(pkg_line);
        labels.emplace(pkgname, index_package_label(pkg_line));
    }
    return true;
}

void cmd_update(const std::vector<std::string>& args) {
    (void)args;

    if(!fs::is_directory(CPK_HOME_DIR)) {
        print_message("Home directory does not exists " + CPK_HOME_DIR, RED);
        return;
    }

    if (!cpk_is_privileged_process()) {
        print_message("cpk update must be run as root to refresh the system index under " + CPK_HOME_DIR + ".", RED);
        return;
    }

    const std::string index_file = get_cpkindex_path();
    const bool had_index = fs::exists(index_file);
    if (CPK_VERBOSE) {
        if (!had_index) {
            print_header("Initializing index of available packages", BLUE);
        }
        else {
            print_header("Updating index of available packages", BLUE);
        }
    }

    // Snapshot the previous index so new/updated packages can be reported by
    // diffing CPKINDEX alone: every port label (name#ver-rel.arch) already
    // lives in the index, so .cpk.info metadata is no longer fetched here.
    // Description/URL (the only fields absent from CPKINDEX) are downloaded
    // on demand by the commands that need them (e.g. cpk info).
    std::unordered_map<std::string, std::string> old_labels;
    if (had_index) {
        read_index_labels(index_file, old_labels);
    }

    const std::string index_url = cpk_repo_join("CPKINDEX");
    if (!download_file(index_url, index_file, true)) {
        print_message("Failed to update index file " + index_file, RED);
        return;
    }
    cpk_invalidate_cpkindex_deps_cache();

    std::vector<std::string> new_labels;
    std::vector<std::string> updated_labels;
    std::unordered_set<std::string> seen;

    std::ifstream index_in(index_file);
    if (!index_in.is_open()) {
        print_message("Error opening index file: " + index_file, RED);
        return;
    }

    std::string index_line;
    while (std::getline(index_in, index_line)) {
        const std::string trimmed = trim_index_line(index_line);
        if (!cpk_index_line_valid(trimmed)) {
            continue;
        }
        const std::string pkg_line = cpk_index_line_package(trimmed);

        // Only the first index row per port name is the newest revision.
        const std::string pkgname = index_pkgname(pkg_line);
        if (!seen.insert(pkgname).second) {
            continue;
        }

        const std::string label = index_package_label(pkg_line);
        auto it = old_labels.find(pkgname);
        if (it == old_labels.end()) {
            new_labels.push_back(label);
        } else if (it->second != label) {
            updated_labels.push_back(label);
        }
    }
    index_in.close();

    // Preserve CPKINDEX line order (do not sort): index order is significant for tooling.

    int package_count = get_number_of_packages();
    print_message("Packages available: " + std::to_string(package_count));

    // On the first initialization every port is trivially new; only report the
    // new/updated breakdown against a pre-existing index.
    if (!had_index) {
        return;
    }
    if (!new_labels.empty()) {
        print_message("New packages: " + std::to_string(new_labels.size()));
        for (const std::string& p : new_labels) {
            print_message("- " + p);
        }
    }
    if (!updated_labels.empty()) {
        print_message("Updated packages: " + std::to_string(updated_labels.size()));
        for (const std::string& p : updated_labels) {
            print_message("- " + p);
        }
    }
}
