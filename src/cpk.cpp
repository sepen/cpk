#include "cpk.h"
#include "utils.h"
#include <fstream>
#include <iostream>
#include <filesystem>

namespace fs = std::filesystem;

std::string CPK_CONF_FILE = "/etc/cpk.conf";
std::string CPK_REPO_URL = "https://cpk.user.ninja";
std::string CPK_HOME_DIR = "/var/lib/cpk";
std::string CPK_INSTALL_ROOT = "/";

bool CPK_COLOR_MODE = true;
bool CPK_VERBOSE = false;

int main(int argc, char* argv[]) {
    // Parse command-line arguments
    std::vector<std::string> command_args;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if ((arg == "--config-file" || arg == "-c") && i + 1 < argc) {
            CPK_CONF_FILE = argv[++i];  // Increment i after consuming argument
        } else if ((arg == "--root" || arg == "-r") && i + 1 < argc) {
            CPK_INSTALL_ROOT = argv[++i];
        } else if ((arg == "--verbose" || arg == "-v")) {
            CPK_VERBOSE = true;
        } else {
            command_args.push_back(arg);  // Collect remaining command arguments
        }
    }

    // If no command is provided, print help
    if (command_args.empty()) {
        print_help();
        return 1;
    }

    // Load the configuration file
    if (!load_cpk_config(CPK_CONF_FILE)) {
        if (CPK_VERBOSE) {
            print_message("Failed to load config file " + CPK_CONF_FILE, RED);
        }
        return 1;
    }

    // Get the first argument as the command and the rest as its arguments
    std::string command = command_args[0];
    std::vector<std::string> args(command_args.begin() + 1, command_args.end());

    // Execute the corresponding command
    if (command == "update") {
        cmd_update(args);
    } else if (command == "info") {
        cmd_info(args);
    } else if (command == "search") {
        cmd_search(args);
    } else if (command == "install" || command == "add") {
        cmd_install(args);
    } else if (command == "uninstall" || command == "del") {
        cmd_uninstall(args);
    } else if (command == "upgrade") {
        cmd_upgrade(args);
    } else if (command == "list") {
        cmd_list(args);
    } else if (command == "build") {
        cmd_build(args);
    } else if (command == "verify") {
        cmd_verify(args);
    } else if (command == "clean") {
        cmd_clean(args);
    } else {
        print_help();
    }

    return 0;
}

// Function to update the list of available packages
void cmd_update(const std::vector<std::string>& args) {
    const std::string index_url = CPK_REPO_URL + "/CPKINDEX";
    const std::string index_file = CPK_HOME_DIR + "/CPKINDEX";

    print_message("Updating index of available packages", BLUE);

    if (!download_file(index_url, index_file, true)) {
        print_message("Failed to update index file " + index_file, RED);
        return;
    }

    int package_count = get_number_of_packages();
    print_message(std::to_string(package_count)+ " packages available", NONE);
}

// Function to display package information
void cmd_info(const std::vector<std::string>& args) {
    if (args.empty()) {
        print_message("Package name is required", RED);
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

    std::string pkgfile_path = package_source + "/Pkgfile";
    std::string pkgdesc, pkgurl, pkgdeps;
    if (!fs::exists(pkgfile_path) || !parse_pkgfile(pkgfile_path, pkgname, pkgdesc, pkgurl, pkgdeps)) {
        print_message("Failed to parse Pkgfile", RED);
        return;
    }

    print_message("Name         | " + pkgname, NONE);
    print_message("Version      | " + pkgver, NONE);
    print_message("Arch         | " + pkgarch, NONE);
    print_message("Description  | " + ltrim(pkgdesc), NONE);
    print_message("URL          | " + ltrim(pkgurl), NONE);
    print_message("Dependencies | " + ltrim(pkgdeps), NONE);
}

// Function to search for a package or description
void cmd_search(const std::vector<std::string>& args) {
    if (args.empty()) {
        print_message("Search argument is required", NONE);
        return;
    }

    std::string search_term = args[0];
    std::string index_file = CPK_HOME_DIR + "/CPKINDEX";

    if (!fs::exists(index_file)) {
        print_message("Package index not found. Run `cpk update` first", RED);
        return;
    }

    std::ifstream file(index_file);
    std::string index_line;
    bool found = false;

    while (std::getline(file, index_line)) {
        if (index_line.find(search_term) != std::string::npos) {
            found = true;
            print_message(index_line, NONE);
        }
    }

    if (!found) {
        print_message("No matching packages found", NONE);
    }

    file.close();
    return;
}

// Function to install a package
void cmd_install(const std::vector<std::string>& args) {
    if (args.empty()) {
        print_message("Package name is required", RED);
        return;
    }

    std::string package, pkgname, pkgver, pkgarch;
    if (!find_package(args[0], package, pkgname, pkgver, pkgarch)) return;

    if (is_package_installed(pkgname)) {
        print_message("Package is already installed", RED);
        return;
    }

    std::string package_url = CPK_REPO_URL + "/" + url_encode(package);
    std::string package_source = CPK_HOME_DIR + "/" + pkgname + "/" + pkgver;
    std::string package_path = CPK_HOME_DIR + "/" + package;

    if (!fs::is_directory(package_source)) {
        if (!download_file(package_url, package_path) || !extract_package(package_path, CPK_HOME_DIR)) {
            print_message("Failed to retrieve package info", RED);
            return;
        }
    }

    std::string package_file = find_pkg_file(package_source, pkgname, pkgver);
    if (!fs::exists(package_file)) {
        print_message("Package file not found", RED);
        return;
    }

    if (CPK_INSTALL_ROOT != "/") {
        if (!run_script(package_source + "/pre-install", "Running pre-install")) {
            print_message("Pre-install script failed", RED);
            return;
        }
        else {
            print_message("Skipping pre-install script", NONE);
        }
    }

    print_message("Installing " + package, BLUE);
    if (shellcmd("pkgadd", {package_file, "-r", CPK_INSTALL_ROOT}, nullptr) != 0) {
        print_message("Failed to install package", RED);
        return;
    }

    if (CPK_INSTALL_ROOT != "/") {
        if (!run_script(package_source + "/post-install", "Running post-install")) {
            print_message("Post-install script failed", RED);
            return;
        }
        else {
            print_message("Skipping post-install script", NONE);
        }
    }

    std::string readme_path = package_source + "/README";
    if (fs::exists(readme_path)) {
        print_message("Printing README", BLUE);
        if (shellcmd("cat", {readme_path}, nullptr) != 0) {
            print_message("Failed to print README file", RED);
            return;
        }
    }

    print_message("Package installed successfully", NONE);
    return;
}

// Function to uninstall a package
void cmd_uninstall(const std::vector<std::string>& args) {
    std::string pkgrm_cmd = "pkgrm";
    std::vector<std::string> pkgrm_args = args;
    std::string pkgrm_output;
    std::string package_name = args[0];

    if (!is_package_installed(package_name)) {
        print_message("Package " + package_name + " not installed", RED);
        return;
    }

    shellcmd(pkgrm_cmd, pkgrm_args, &pkgrm_output, false);
    return;
}

// Function to list installed packages
void cmd_list(const std::vector<std::string>& args) {
    std::string pkginfo_cmd = "pkginfo";
    std::vector<std::string> pkginfo_args = { "-i" };
    std::string pkginfo_output;

    shellcmd(pkginfo_cmd, pkginfo_args, &pkginfo_output);
    return;
}

// Function to upgrade installed packages
void cmd_upgrade(const std::vector<std::string>& args) {
    print_message("Not implemented yet", YELLOW);
    return;
}

// Function to build a package using pkgmk
void cmd_build(const std::vector<std::string>& args) {
    print_message("Not implemented yet", YELLOW);
    return;
}

// Function to check the integrity of packages against their stored checksums and signatures
void cmd_verify(const std::vector<std::string>& args) {
    if (args.empty()) {
        print_message("Package name is required", RED);
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

    // Find public keys in /etc/ports/
    std::vector<std::string> pub_keys = find_public_keys("/etc/ports/");
    if (pub_keys.empty()) {
        print_message("No public keys found in /etc/ports/", RED);
        return;
    }

    // Download missing source files
    std::string pkgmk_cmd = "pkgmk";
    std::vector<std::string> pkgmk_args = { "-do" };
    std::string pkgmk_output;

    int ret = shellcmd(pkgmk_cmd, pkgmk_args, &pkgmk_output, false);

    if (ret != 0) {
        print_message("Failed to download missing source files", RED);
        return;
    }

    // Try each public key until one works
    for (const std::string& public_key : pub_keys) {
        std::string signature_cmd = "signify";
        std::vector<std::string> signature_args = { "-q", "-C", "-p", public_key, "-x", ".signature" };
        std::string signature_output;

        int ret = shellcmd(signature_cmd, signature_args, &signature_output, false);

        if (ret == 0) {
            print_message("Verification successful with key: " + public_key, NONE);
            return; // Stop on first success
        }
    }

    print_message("Verification failed for all keys in /etc/ports/", RED);
}

// Function to clean cache contents
void cmd_clean(const std::vector<std::string>& args) {

    std::string excluded_file = "CPKINDEX";

    if (fs::exists(CPK_HOME_DIR) && fs::is_directory(CPK_HOME_DIR)) {
        // Iterate over directory contents and remove them
        for (const auto& entry : fs::directory_iterator(CPK_HOME_DIR)) {
            if (entry.path().filename() == excluded_file) {
                continue;
            }
            fs::remove_all(entry);
        }
        print_message("Cache contents deleted successfully", NONE);
    } else {
        print_message("Path does not exists or is not a directory " + CPK_HOME_DIR, RED);
    }

    return;
}
