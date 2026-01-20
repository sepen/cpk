#include "../cpk.h"
#include "../utils.h"
#include <filesystem>
#include <vector>
#include <string>

namespace fs = std::filesystem;

void cmd_build(const std::vector<std::string>& args) {
    if (args.empty()) {
        print_message("Package name is required", RED);
        return;
    }

    std::string index_file = CPK_HOME_DIR + "/CPKINDEX";
    if (!fs::exists(index_file)) {
        print_message("Package index not found. Run `cpk update` first", RED);
        return;
    }

    std::string package, pkgname, pkgver, pkgarch;
    if (!find_package(args[0], package, pkgname, pkgver, pkgarch)) return;

    std::string package_url = CPK_REPO_URL + "/" + url_encode(package);
    std::string package_source = CPK_HOME_DIR + "/" + pkgname + "/" + pkgver;
    std::string package_path = CPK_HOME_DIR + "/" + package;

    if (!fs::is_directory(package_source)) {
        if (!download_file(package_url, package_path) || !extract_package(package_path, CPK_HOME_DIR)) {
            print_message("Failed to retrieve package info", RED);
            return;
        }
    }

    // Change to the package source directory
    if (!change_directory(package_source)) {
        print_message("Failed to change directory to: " + package_source, RED);
        return;
    }

    // Build the package
    std::vector<std::string> pkgmk_args = { "-d" };
    std::string pkgmk_output;
    if (CPK_VERBOSE) {
        print_header("Running '" + CPK_PKGMK_CMD + "' in " + package_source);
    }
    int ret = shellcmd(CPK_PKGMK_CMD, pkgmk_args, &pkgmk_output, true);

    if (ret != 0) {
        print_message("Failed to build package", RED);
        return;
    }

    print_message("Package built successfully");
}
