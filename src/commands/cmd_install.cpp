#include "../cpk.h"
#include "../utils.h"
#include <filesystem>
#include <vector>
#include <string>
#include <set>

namespace fs = std::filesystem;

static void parse_install_flags(const std::vector<std::string>& args,
                                std::vector<std::string>& positional,
                                bool& upgrade,
                                bool& no_deps) {
    upgrade = false;
    no_deps = false;
    positional.clear();
    for (const auto& a : args) {
        if (a == "--upgrade") {
            upgrade = true;
        } else if (a == "--no-deps") {
            no_deps = true;
        } else {
            positional.push_back(a);
        }
    }
}

static bool spec_resolves_installed(const std::string& spec) {
    if (fs::exists(spec) && fs::is_regular_file(spec)) {
        std::string pn, pv, pa;
        if (!parse_cpk_filename(spec, pn, pv, pa)) {
            return false;
        }
        return is_package_installed(pn);
    }
    std::string package, pkgname, pkgver, pkgarch;
    if (!find_package(spec, package, pkgname, pkgver, pkgarch)) {
        return false;
    }
    return is_package_installed(pkgname);
}

static bool visit_for_plan(const std::string& spec,
                           std::vector<std::string>& plan,
                           std::set<std::string>& visit_stack,
                           std::set<std::string>& done) {
    if (done.count(spec)) {
        return true;
    }
    if (visit_stack.count(spec)) {
        return true;
    }
    visit_stack.insert(spec);
    std::vector<std::string> deps;
    if (!get_package_dependency_names(spec, deps)) {
        visit_stack.erase(spec);
        return false;
    }
    for (const auto& d : deps) {
        if (!visit_for_plan(d, plan, visit_stack, done)) {
            visit_stack.erase(spec);
            return false;
        }
    }
    visit_stack.erase(spec);
    done.insert(spec);
    plan.push_back(spec);
    return true;
}

// Install a single package spec (repo name, name#ver, or path to .cpk). Returns false on hard failure.
static bool install_package_spec(const std::string& spec, bool allow_upgrade_if_installed) {
    std::string package, pkgname, pkgver, pkgarch;
    std::string package_file;
    bool is_local_file = false;

    if (fs::exists(spec) && fs::is_regular_file(spec)) {
        if (parse_cpk_filename(spec, pkgname, pkgver, pkgarch)) {
            is_local_file = true;
            package_file = spec;
            package = fs::path(spec).filename().string();
        } else {
            print_message("Invalid .cpk file format: " + spec, RED);
            return false;
        }
    } else {
        std::string index_file = get_cpkindex_path();
        if (!fs::exists(index_file)) {
            print_message("Package index not found. Run `cpk update` first", RED);
            return false;
        }

        if (!find_package(spec, package, pkgname, pkgver, pkgarch)) {
            return false;
        }
    }

    std::string upgrade_flag;
    if (is_package_installed(pkgname)) {
        if (allow_upgrade_if_installed) {
            print_header("Upgrading package " + pkgname, BLUE);
            upgrade_flag = "-u";
        } else {
            print_message("Package " + pkgname + " is already installed", YELLOW);
            return true;
        }
    } else {
        print_header("Installing package " + pkgname, BLUE);
        upgrade_flag = "";
    }

    std::string cache_dir = get_cache_dir();
    std::string package_source = cache_dir + "/" + pkgname + "/" + pkgver;

    if (is_local_file) {
        if (!fs::is_directory(package_source)) {
            if (!extract_package(package_file, cache_dir)) {
                print_message("Failed to extract package sources", RED);
                return false;
            }
        }
    } else {
        std::string package_url = cpk_repo_join(url_encode(package));
        std::string package_path = get_cache_file(package);

        if (!fs::is_directory(package_source)) {
            if (!download_file(package_url, package_path) || !extract_package(package_path, cache_dir)) {
                print_message("Failed to retrieve package sources", RED);
                return false;
            }
        }

        package_file = find_pkg_file(package_source, pkgname, pkgver);
        if (!fs::exists(package_file)) {
            print_message("Package file not found", YELLOW);
            return false;
        }
    }

    run_script(package_source + "/pre-install", "Running pre-install script");

    std::vector<std::string> pkgadd_args = { "-r", CPK_INSTALL_ROOT, upgrade_flag, package_file };
    std::string pkgadd_output;

    if (CPK_VERBOSE) {
        print_message("Running " + CPK_PKGADD_CMD + " " + pkgadd_args[0] + " " + pkgadd_args[1] + " " + pkgadd_args[2] + " " + pkgadd_args[3]);
    }

    if (shellcmd(CPK_PKGADD_CMD, pkgadd_args, &pkgadd_output) != 0) {
        print_message("Failed to install package", RED);
        return false;
    }

    run_script(package_source + "/post-install", "Running post-install script");

    std::string readme_path = package_source + "/README";
    if (fs::exists(readme_path)) {
        print_header("Printing package's README file", BLUE);
        if (shellcmd("cat", { readme_path }, nullptr) != 0) {
            print_message("Failed to print package's README file", RED);
            return false;
        }
    }

    print_message("Package installed successfully");
    return true;
}

void cmd_install(const std::vector<std::string>& args) {
    std::vector<std::string> positional;
    bool upgrade = false;
    bool no_deps = false;
    parse_install_flags(args, positional, upgrade, no_deps);

    if (positional.empty()) {
        print_message("Package name or path to .cpk file is required", RED);
        return;
    }

    const std::string& primary = positional[0];

    if (!no_deps) {
        std::vector<std::string> plan;
        std::set<std::string> visit_stack, done;
        if (!visit_for_plan(primary, plan, visit_stack, done)) {
            print_message("Failed to resolve dependency tree", RED);
            return;
        }
        for (const auto& spec : plan) {
            const bool allow_upgrade = upgrade && (spec == primary);
            if (spec_resolves_installed(spec) && !allow_upgrade) {
                continue;
            }
            if (!install_package_spec(spec, allow_upgrade)) {
                return;
            }
        }
        return;
    }

    if (!install_package_spec(primary, upgrade)) {
        return;
    }
}
