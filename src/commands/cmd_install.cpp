#include "../cpk.h"
#include "../utils.h"
#include <filesystem>
#include <vector>
#include <string>

namespace fs = std::filesystem;

void cmd_install(const std::vector<std::string>& args) {
    if (args.empty()) {
        print_message("Package name or path to .cpk file is required", RED);
        return;
    }

    std::string package, pkgname, pkgver, pkgarch;
    std::string package_file;
    bool is_local_file = false;
    
    // Check if argument is a path to a local .cpk file
    if (fs::exists(args[0]) && fs::is_regular_file(args[0])) {
        // Try to parse the filename
        if (parse_cpk_filename(args[0], pkgname, pkgver, pkgarch)) {
            is_local_file = true;
            package_file = args[0];
            package = fs::path(args[0]).filename().string();
        } else {
            print_message("Invalid .cpk file format: " + args[0], RED);
            return;
        }
    } else {
        // Normal package installation from repository
        std::string index_file = CPK_HOME_DIR + "/CPKINDEX";
        if (!fs::exists(index_file)) {
            print_message("Package index not found. Run `cpk update` first", RED);
            return;
        }

        if (!find_package(args[0], package, pkgname, pkgver, pkgarch)) return;
    }

    std::string upgrade_flag;
    if (is_package_installed(pkgname)) {
        if (args.size() > 1 && args[1] == "--upgrade") {
            print_header("Upgrading package " + pkgname, BLUE);
            upgrade_flag = "-u";
        }
        else {
            print_message("Package " + pkgname + " is already installed", YELLOW);
            return;
        }
    }
    else {
        print_header("Installing package " + pkgname, BLUE);
        upgrade_flag = "";
    }

    std::string cache_dir = get_cache_dir();
    std::string package_source = cache_dir + "/" + pkgname + "/" + pkgver;
    
    if (is_local_file) {
        // For local files, extract to cache directory to get pre/post install scripts and README
        if (!fs::is_directory(package_source)) {
            if (!extract_package(package_file, cache_dir)) {
                print_message("Failed to extract package sources", RED);
                return;
            }
        }
    } else {
        // We need to download and extract the package if not already done
        // That way we can run pre/post install scripts and show README
        std::string package_url = CPK_REPO_URL + "/" + url_encode(package);
        std::string package_path = get_cache_file(package);

        if (!fs::is_directory(package_source)) {
            if (!download_file(package_url, package_path) || !extract_package(package_path, cache_dir)) {
                print_message("Failed to retrieve package sources", RED);
                return;
            }
        }

        package_file = find_pkg_file(package_source, pkgname, pkgver);
        if (!fs::exists(package_file)) {
            print_message("Package file not found", YELLOW);
            return;
        }
    }

    // Run pre-install script if exists
    run_script(package_source + "/pre-install", "Running pre-install script");

    // Install the package
    std::vector<std::string> pkgadd_args = { "-r", CPK_INSTALL_ROOT, upgrade_flag, package_file };
    std::string pkgadd_output;

    if (CPK_VERBOSE) {
        print_message("Running " + CPK_PKGADD_CMD + " " + pkgadd_args[0] + " " + pkgadd_args[1] + " " + pkgadd_args[2] + " " + pkgadd_args[3]);
    }

    if (shellcmd(CPK_PKGADD_CMD, pkgadd_args, &pkgadd_output) != 0) {
        print_message("Failed to install package", RED);
        return;
    }

    // Run post-install script if exists
    run_script(package_source + "/post-install", "Running post-install script");

    // Print contents of README if exists
    std::string readme_path = package_source + "/README";
    if (fs::exists(readme_path)) {
        print_header("Printing package's README file", BLUE);
        if (shellcmd("cat", {readme_path}, nullptr) != 0) {
            print_message("Failed to print package's README file", RED);
            return;
        }
    }

    print_message("Package installed successfully");
    return;
}
