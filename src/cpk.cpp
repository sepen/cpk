#include "cpk.h"
#include "utils.h"
#include <fstream>
#include <iostream>
#include <filesystem>
#include <set>

namespace fs = std::filesystem;

const std::string CPK_VERSION = "0.1";

std::string CPK_CONF_FILE = "/etc/cpk.conf";
std::string CPK_REPO_URL = "https://cpk.user.ninja";
std::string CPK_HOME_DIR = "/var/lib/cpk";
std::string CPK_INSTALL_ROOT = "/";
std::string CPK_PKGMK_CMD = "pkgmk";
std::string CPK_PKGADD_CMD = "pkgadd";
std::string CPK_PKGRM_CMD = "pkgrm";
std::string CPK_PKGINFO_CMD = "pkginfo";

bool CPK_COLOR_MODE = false;
bool CPK_VERBOSE = false;

int main(int argc, char* argv[]) {
    // Parse command-line arguments
    std::vector<std::string> command_args;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if ((arg == "--config" || arg == "-c") && i + 1 < argc) {
            CPK_CONF_FILE = argv[++i];  // Increment i after consuming argument
        } else if ((arg == "--root" || arg == "-r") && i + 1 < argc) {
            CPK_INSTALL_ROOT = argv[++i];
        } else if ((arg == "--color" || arg == "-C")) {
            CPK_COLOR_MODE = true;
        } else if ((arg == "--verbose" || arg == "-v")) {
            CPK_VERBOSE = true;
        } else if (arg == "--help" || arg == "-h") {
            print_help();
            return 0;
        } else if (arg == "--version" || arg == "-V") {
            print_version();
            return 0;
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
    } else if (command == "list") {
        cmd_list(args);
    } else if (command == "diff") {
        cmd_diff(args);
    } else if (command == "verify") {
        cmd_verify(args);
    } else if (command == "build") {
        cmd_build(args);
    } else if (command == "install" || command == "add") {
        cmd_install(args);
    } else if (command == "uninstall" || command == "del") {
        cmd_uninstall(args);
    } else if (command == "upgrade") {
        cmd_upgrade(args);
    } else if (command == "clean") {
        cmd_clean(args);
    } else if (command == "index") {
        cmd_index(args);
    } else if (command == "archive") {
        cmd_archive(args);
    } else if (command == "version") {
        print_version();
    } else {
        print_help();
    }

    return 0;
}

// Function to update the list of available packages
void cmd_update(const std::vector<std::string>& args) {
    if(!fs::is_directory(CPK_HOME_DIR)) {
        print_message("Home directory does not exists " + CPK_HOME_DIR, RED);
        return;
    }

    const std::string index_file = CPK_HOME_DIR + "/CPKINDEX";
    if (CPK_VERBOSE) {
        if (!fs::exists(index_file)) {
            print_header("Initializing index of available packages", BLUE);
        }
        else {
            print_header("Updating index of available packages", BLUE);
        }
    }

    const std::string index_url = CPK_REPO_URL + "/CPKINDEX";
    if (!download_file(index_url, index_file, true)) {
        print_message("Failed to update index file " + index_file, RED);
        return;
    }

    int package_count = get_number_of_packages();
    print_message(std::to_string(package_count)+ " packages available");
}

// Function to display package information
void cmd_info(const std::vector<std::string>& args) {
    if (args.empty()) {
        print_message("Package name is required", YELLOW);
        return;
    }

    const std::string index_file = CPK_HOME_DIR + "/CPKINDEX";
    if (!fs::exists(index_file)) {
        print_message("Package index not found. Run `cpk update` first", YELLOW);
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

    print_message(BOLD + "Name         " + NONE + "| " + pkgname);
    print_message(BOLD + "Version      " + NONE + "| " + pkgver);
    print_message(BOLD + "Arch         " + NONE + "| " + pkgarch);
    print_message(BOLD + "Description  " + NONE + "| " + ltrim(pkgdesc));
    print_message(BOLD + "URL          " + NONE + "| " + ltrim(pkgurl));
    print_message(BOLD + "Dependencies " + NONE + "| " + ltrim(pkgdeps));
}

// Function to search for a package or description
void cmd_search(const std::vector<std::string>& args) {
    if (args.empty()) {
        print_message("Search argument is required");
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

    std::ostringstream search_results;
    while (std::getline(file, index_line)) {
        if (index_line.find(search_term) != std::string::npos) {
            found = true;
            search_results << index_line << '\n';
        }
    }
    file.close();

    if (!found) {
        print_message("No matching packages found", YELLOW);
    } else {
        print_fmt_lines(search_results.str());
    }

    return;
}

// Function to list installed packages
void cmd_list(const std::vector<std::string>& args) {
    std::vector<std::string> pkginfo_args = { "-i" };
    std::string pkginfo_output;

    if (shellcmd(CPK_PKGINFO_CMD, pkginfo_args, &pkginfo_output, false) != 0) {
        print_message("Failed to get list of installed packages", RED);
    }
    else {
        if (CPK_VERBOSE) {
            print_header("Printing list of installed packages", BLUE);
        }
        print_fmt_header("Package Version");
        print_fmt_lines(pkginfo_output);
    }

    return;
}

// Function to compare installed and available packages
void cmd_diff(const std::vector<std::string>& args) {
    // Get installed packages
    std::vector<std::string> pkginfo_args = { "-i" };
    std::string installed_packages;
    shellcmd(CPK_PKGINFO_CMD, pkginfo_args, &installed_packages, false);

    // Compare and display differences
    std::istringstream installed_stream(installed_packages);
    std::string diff_packages;
    bool found_difference = false;

    // Read installed packages line by line and split into name & version
    std::string installed_pkgname, installed_pkgver;
    std::string package, pkgname, pkgver, pkgarch;
    while (installed_stream >> installed_pkgname >> installed_pkgver) {
        if (find_package(installed_pkgname, package, pkgname, pkgver, pkgarch)) {
            if (installed_pkgname == pkgname && installed_pkgver != pkgver) {
                found_difference = true;
                diff_packages += installed_pkgname + " " + installed_pkgver + " " + pkgver + "\n";
            }
        }
    }

    if (!found_difference) {
        print_message("No differences found", GREEN);
    }
    else {
        if (CPK_VERBOSE) {
            print_header("Differences between installed and available packages", BLUE);
        }
        print_fmt_header("Package Installed Available");
        print_fmt_lines(diff_packages);
    }

    return;
}

// Function to check the integrity of packages against their stored checksums and signatures
void cmd_verify(const std::vector<std::string>& args) {
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

    // Find public keys in /etc/ports/
    std::vector<std::string> pub_keys = find_public_keys("/etc/ports/");
    if (pub_keys.empty()) {
        print_message("No public keys found in /etc/ports/", RED);
        return;
    }

    // Download missing source files
    std::vector<std::string> pkgmk_args = { "-do" };
    std::string pkgmk_output;

    int ret = shellcmd(CPK_PKGMK_CMD, pkgmk_args, &pkgmk_output, false);

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
            print_message("Verification successful with key: " + public_key);
            return; // Stop on first success
        }
    }

    print_message("Verification failed for all keys in /etc/ports/", RED);
}

// Function to build a package using pkgmk
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

// Function to install a package
void cmd_install(const std::vector<std::string>& args) {
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

    if (is_package_installed(pkgname)) {
        if (args[1] != "--force") {
            print_message("Package is already installed", YELLOW);
            return;
        }
        else {
            print_header("Upgrading package " + pkgname, BLUE);
        }
    }

    print_header("Installing package " + pkgname, BLUE);

    std::string package_url = CPK_REPO_URL + "/" + url_encode(package);
    std::string package_source = CPK_HOME_DIR + "/" + pkgname + "/" + pkgver;
    std::string package_path = CPK_HOME_DIR + "/" + package;

    if (!fs::is_directory(package_source)) {
        if (!download_file(package_url, package_path) || !extract_package(package_path, CPK_HOME_DIR)) {
            print_message("Failed to retrieve package sources", RED);
            return;
        }
    }

    std::string package_file = find_pkg_file(package_source, pkgname, pkgver);
    if (!fs::exists(package_file)) {
        print_message("Package file not found", YELLOW);
        return;
    }

    // Run pre-install script if exists
    run_script(package_source + "/pre-install", "Running pre-install script");

    // Install the package
    std::vector<std::string> pkgadd_args = { "-r", CPK_INSTALL_ROOT, package_file };
    std::string pkgadd_output;

    if (CPK_VERBOSE) {
        print_message("Running " + CPK_PKGADD_CMD + " -r " + CPK_INSTALL_ROOT + " " + pkgname);
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

// Function to uninstall a package
void cmd_uninstall(const std::vector<std::string>& args) {

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

    if (!is_package_installed(pkgname)) {
        print_message("Package " + pkgname + " not installed", RED);
        return;
    }

    print_header("Uninstalling package " + pkgname, BLUE);

    std::vector<std::string> pkgrm_args = { "-r", CPK_INSTALL_ROOT, pkgname };
    std::string pkgrm_output;

    if (CPK_VERBOSE) {
        print_message("Running " + CPK_PKGRM_CMD + " -r " + CPK_INSTALL_ROOT + " " + pkgname);
    }

    if (shellcmd(CPK_PKGRM_CMD, pkgrm_args, &pkgrm_output) != 0) {
        print_message("Failed to uninstall package", RED);
        return;
    }
    return;
}

// Function to upgrade installed packages
void cmd_upgrade(const std::vector<std::string>& args) {
    print_message("Not implemented yet", YELLOW);
    return;
}

// Function to clean cache contents
void cmd_clean(const std::vector<std::string>& args) {

    std::string excluded_file = "CPKINDEX";

    if (CPK_VERBOSE) {
        print_header("Cleaning cache contents", BLUE);
    }

    if (fs::exists(CPK_HOME_DIR) && fs::is_directory(CPK_HOME_DIR)) {
        // Iterate over directory contents and remove them
        for (const auto& entry : fs::directory_iterator(CPK_HOME_DIR)) {
            if (entry.path().filename() == excluded_file) {
                continue;
            }
            fs::remove_all(entry);
        }
        print_message("Cache contents deleted successfully");
    } else {
        print_message("Path does not exists or is not a directory " + CPK_HOME_DIR, RED);
    }

    return;
}

// Function to parse a Pkgfile and extract package details
void cmd_index(const std::vector<std::string>& args) {
    if (args.size() != 1) {
        return;
    }
    fs::path repo_dir = args[0];
    if (!fs::is_directory(repo_dir)) {
        print_message("Directory does not exist: " + repo_dir.string(), RED);
        return;
    }
    if (CPK_VERBOSE) {
        print_header("Updating index of local repository", BLUE);
    }
    generate_cpk_index(repo_dir);
    return;
}

// Function to archive packages from a ports directory
void cmd_archive(const std::vector<std::string>& args) {
    if (args.size() != 2) {
        print_message("Usage: cpk archive <prtdir> <repo>", YELLOW);
        return;
    }
    fs::path ports_dir = args[0];
    fs::path output_dir = args[1];
    ensure_directory(output_dir);

    std::string arch = get_system_architecture();

    if (CPK_VERBOSE) {
        print_header("Creating .cpk archive(s) from ports in " + ports_dir.string(), BLUE);
    }

    if (!fs::is_directory(ports_dir)) {
        print_message("Directory does not exist: " + ports_dir.string(), RED);
        return;
    }

    // Iterate over port directories
    for (const auto &entry : fs::recursive_directory_iterator(ports_dir)) {
        //if (entry.path().extension().string().find(".pkg.") != std::string::npos) {

        if (!entry.is_regular_file()) continue;

        fs::path package_path = entry.path();
        std::string package = package_path.filename().string();

        // Detect known compressed pkg formats
        if ((package.ends_with(".pkg.tar.gz") || package.ends_with(".pkg.tar.bz2") || package.ends_with(".pkg.tar.xz"))) {
            if (CPK_VERBOSE) {
                print_message("Processing package file: " + entry.path().string());
            }

            std::string package_prefix = package.substr(0, package.find(".pkg."));
            fs::path package_dir = package_path.parent_path();
            std::ifstream pkgfile(package_dir / "Pkgfile");
            if (!pkgfile) continue;

            std::string name, version, release;
            std::vector<std::string> sources;
            std::string line;
            while (std::getline(pkgfile, line)) {
                if (line.find("name=") == 0) name = line.substr(5);
                else if (line.find("version=") == 0) version = line.substr(8);
                else if (line.find("release=") == 0) release = line.substr(8);
                else if (line.find("source=(") == 0) {
                    line = line.substr(7, line.length() - 8); // Remove 'source=(' and ')'
                    sources.push_back(line);
                }
            }

            if (package_prefix == name + "#" + version + "-" + release) {
                print_message("Packaging " + output_dir.string() + "/" + package_prefix + "." + arch + ".cpk");
                fs::path basedir = output_dir / name / (version + "-" + release);
                ensure_directory(basedir);
                auto local_files = get_local_files(sources);
                copy_files(package_dir, basedir, local_files);
                fs::copy(package_path, basedir / package, fs::copy_options::overwrite_existing);
                package_files(name, version, release, arch, output_dir);
            }
        }
    }
    return;
}