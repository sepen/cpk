#include "../cpk.h"
#include "../utils.h"
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

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

// CPKINDEX line "name#ver-rel.arch.cpk" -> unique label without ".cpk"
static std::string index_package_label(const std::string& pkg_line) {
    if (pkg_line.size() >= 4 && pkg_line.compare(pkg_line.size() - 4, 4, ".cpk") == 0) {
        return pkg_line.substr(0, pkg_line.size() - 4);
    }
    return pkg_line;
}

void cmd_update(const std::vector<std::string>& args) {
    if(!fs::is_directory(CPK_HOME_DIR)) {
        print_message("Home directory does not exists " + CPK_HOME_DIR, RED);
        return;
    }

    const std::string index_file = get_cpkindex_path();
    if (CPK_VERBOSE) {
        if (!fs::exists(index_file)) {
            print_header("Initializing index of available packages", BLUE);
        }
        else {
            print_header("Updating index of available packages", BLUE);
        }
    }

    const std::string index_url = cpk_repo_join("CPKINDEX");
    if (!download_file(index_url, index_file, true)) {
        print_message("Failed to update index file " + index_file, RED);
        return;
    }

    if (CPK_VERBOSE) {
        print_header("Syncing .cpk.info metadata for indexed packages", BLUE);
    }

    int new_info = 0;
    int version_changes = 0;
    int info_failures = 0;
    std::vector<std::string> new_labels;
    std::vector<std::string> updated_labels;

    std::ifstream index_in(index_file);
    if (!index_in.is_open()) {
        print_message("Error opening index file: " + index_file, RED);
        return;
    }

    std::string index_line;
    while (std::getline(index_in, index_line)) {
        const std::string pkg_line = trim_index_line(index_line);
        if (pkg_line.empty()) {
            continue;
        }
        if (pkg_line.size() < 5 || pkg_line.compare(pkg_line.size() - 4, 4, ".cpk") != 0) {
            continue;
        }

        const std::string info_path = get_cache_file(pkg_line + ".info");
        const bool had_file = fs::exists(info_path);
        std::string old_name, old_ver, old_arch, old_desc, old_url, old_deps;
        bool old_ok = false;
        if (had_file) {
            old_ok = parse_cpk_info(info_path, old_name, old_ver, old_arch, old_desc, old_url, old_deps);
        }

        // Valid local .cpk.info: keep it (no network fetch)
        if (old_ok) {
            continue;
        }

        const std::string info_url = cpk_repo_join(url_encode(pkg_line + ".info"));
        const std::string tmp_path = info_path + ".tmp";
        if (fs::exists(tmp_path)) {
            fs::remove(tmp_path);
        }
        if (!download_file(info_url, tmp_path, true)) {
            if (fs::exists(tmp_path)) {
                fs::remove(tmp_path);
            }
            ++info_failures;
            continue;
        }

        std::string name, ver, arch, desc, url, deps;
        if (!parse_cpk_info(tmp_path, name, ver, arch, desc, url, deps)) {
            fs::remove(tmp_path);
            ++info_failures;
            continue;
        }

        try {
            fs::rename(tmp_path, info_path);
        } catch (const fs::filesystem_error&) {
            fs::remove(tmp_path);
            ++info_failures;
            continue;
        }

        const std::string label = index_package_label(pkg_line);

        if (!had_file) {
            ++new_info;
            new_labels.push_back(label);
        } else if (old_ok && ver != old_ver) {
            ++version_changes;
            updated_labels.push_back(label);
        }
    }
    index_in.close();

    // Preserve CPKINDEX line order (do not sort): index order is significant for tooling.

    int package_count = get_number_of_packages();
    print_message("Packages available: " + std::to_string(package_count));
    if (new_info > 0) {
        print_message("New packages: " + std::to_string(new_info));
        for (const std::string& p : new_labels) {
            print_message("- " + p);
        }
    }
    if (version_changes > 0) {
        print_message("Updated packages: " + std::to_string(version_changes));
        for (const std::string& p : updated_labels) {
            print_message("- " + p);
        }
    }
    if (info_failures > 0) {
        print_message(std::to_string(info_failures) + " .cpk.info sync failures (missing or invalid on server?)", YELLOW);
    }
}
