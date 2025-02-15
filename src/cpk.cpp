#include "cpk.h"
#include "utils.h"
#include <fstream>
#include <iostream>
#include <filesystem>

namespace fs = std::filesystem;

std::string CPK_CONF_FILE = "/etc/cpk.conf";
std::string CPK_REPO_URL = "https://cpk.user.ninja";
std::string CPK_HOME_DIR = "/var/lib/cpk";
bool CPK_COLOR_MODE = true;

int main(int argc, char* argv[]) {
    std::string config_file = CPK_CONF_FILE;
    
    // Parse command-line arguments
    std::vector<std::string> command_args;
    bool config_parsed = false;

    for (int i = 1; i < argc; ++i) {
        if ((std::string(argv[i]) == "--config-file" || std::string(argv[i]) == "-c") && i + 1 < argc) {
            config_file = argv[i + 1];
            i++; // Skip the next argument as it's the file path
            config_parsed = true; // Mark that we've parsed the config flag
        } else {
            command_args.push_back(argv[i]); // Collect remaining command arguments
        }
    }

    // If no command is provided, print help
    if (command_args.empty()) {
        print_help();
        return 1;
    }

    // Load the configuration file
    if (!load_cpk_config(config_file)) {
        print_message("Failed to load configuration file: " + config_file, RED);
        return 1;
    }

    // Get the first argument as the command and the rest as its arguments
    std::string command = command_args[0];
    std::vector<std::string> args(command_args.begin() + 1, command_args.end());

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
    } else if (command == "clean") {
        cmd_clean(args);
    } else if (command == "build") {
        cmd_build(args);
    } else {
        print_help();
    }

    return 0;
}

// Function to update the list of available packages
void cmd_update(const std::vector<std::string>& args) {
    const std::string index_url = CPK_REPO_URL + "/CPKINDEX";
    const std::string index_file = CPK_HOME_DIR + "/CPKINDEX";

    print_message("Updating index for packages...", BLUE);
    print_message("Fetching: " + index_url, NONE);

    if (download_file(index_url, index_file)) {
        print_message("Update successful", GREEN);
    } else {
        print_message("Failed to update index file: " + index_file, RED);
    }
}

// Function to display package information
void cmd_info(const std::vector<std::string>& args) {
    if (args.empty()) {
        print_message("Package name is required", RED);
        return;
    }

    std::string package_name = args[0];
    std::string index_file = CPK_HOME_DIR + "/CPKINDEX";

    if (!fs::exists(index_file)) {
        print_message("Package index not found. Run `cpk update` first", RED);
        return;
    }

    std::ifstream infile(index_file);
    std::string index_line;
    std::string package;
    std::string pkgname, pkgver, pkgarch, pkgdesc, pkgurl, pkgdeps;
    bool found = false;

    while (std::getline(infile, index_line)) {
        if (index_line.find(package_name) != std::string::npos) {
            found = true;

            size_t cpk_pos = index_line.rfind(".cpk");
            size_t last_dot = index_line.rfind('.', cpk_pos - 1);
            size_t hash_pos = index_line.find('#');

            if (cpk_pos != std::string::npos && last_dot != std::string::npos && hash_pos != std::string::npos) {
                pkgname = index_line.substr(0, hash_pos);
                pkgver = index_line.substr(hash_pos + 1, last_dot - hash_pos - 1);
                pkgarch = index_line.substr(last_dot + 1, cpk_pos - last_dot - 1);
                package = index_line;
            } else {
                print_message("Invalid package format", RED);
            }
        }
    }

    if (!found) {
        print_message("Package not found: " + package_name, RED);
        return;
    }

    std::string package_url = CPK_REPO_URL + "/" + url_encode(package);
    std::string package_path = CPK_HOME_DIR + "/" + package;
    std::string package_source = CPK_HOME_DIR + "/" + pkgname + "/" + pkgver;

    // Fetch package and its source
    if (!fs::is_directory(package_source)) {
        if (download_file(package_url, package_path)) {
            if (extract_package(package_path, CPK_HOME_DIR)) {
                print_message("Package source extracted successfuly", NONE);
            } else {
                print_message("Failed to extract package info: " + package, RED);
                return;
            }
        }
        else {
            print_message("Failed to download package: " + package, RED);
            return;
        }
    }

    std::string pkgfile_path = package_source + "/Pkgfile";
    std::string package_file;

    // Read Pkgfile
    if (!fs::exists(pkgfile_path)) {
        print_message("Pkgfile not found in " + pkgfile_path, RED);
    } else {
        if (!parse_pkgfile(pkgfile_path, pkgname, pkgdesc, pkgurl, pkgdeps)) {
            print_message("Failed to parse Pkgfile", RED);
        } else {

            package_file = find_pkg_file(package_source, pkgname, pkgver);

            if (fs::exists(package_file)) {
                // Display parsed values
                print_message("Name         | " + pkgname, NONE);
                print_message("Version      | " + pkgver, NONE);
                print_message("Arch         | " + pkgarch, NONE);
                print_message("Description  | " + ltrim(pkgdesc), NONE);
                print_message("URL          | " + ltrim(pkgurl), NONE);
                print_message("Dependencies | " + ltrim(pkgdeps), NONE);
            }
        }
    }
}

// Function to search for a package or description
void cmd_search(const std::vector<std::string>& args) {
    if (args.empty()) {
        print_message("Search term is required", RED);
        return;
    }

    std::string search_term = args[0];
    std::string index_file = CPK_HOME_DIR + "/CPKINDEX";

    if (!fs::exists(index_file)) {
        print_message("Package index not found. Run `cpk update` first", RED);
        return;
    }

    std::ifstream infile(index_file);
    std::string index_line;
    bool found = false;

    while (std::getline(infile, index_line)) {
        if (index_line.find(search_term) != std::string::npos) {
            found = true;
            print_message(index_line, NONE);
        }
    }

    if (!found) {
        print_message("No packages found matching: " + search_term, RED);
    }
}

// Function to install a package
void cmd_install(const std::vector<std::string>& args) {
    if (args.empty()) {
        print_message("Package name is required", RED);
        return;
    }

    std::string package_name = args[0];
    std::string index_file = CPK_HOME_DIR + "/CPKINDEX";

    if (!fs::exists(index_file)) {
        print_message("Package index not found. Run `cpk update` first", RED);
        return;
    }

    std::ifstream infile(index_file);
    std::string index_line;
    std::string package;
    std::string pkgname, pkgver, pkgarch, pkgdesc, pkgurl, pkgdeps;
    bool found = false;

    while (std::getline(infile, index_line)) {
        if (index_line.find(package_name) != std::string::npos) {
            found = true;

            size_t cpk_pos = index_line.rfind(".cpk");
            size_t last_dot = index_line.rfind('.', cpk_pos - 1);
            size_t hash_pos = index_line.find('#');

            if (cpk_pos != std::string::npos && last_dot != std::string::npos && hash_pos != std::string::npos) {
                pkgname = index_line.substr(0, hash_pos);
                pkgver = index_line.substr(hash_pos + 1, last_dot - hash_pos - 1);
                pkgarch = index_line.substr(last_dot + 1, cpk_pos - last_dot - 1);
                package = index_line;
            } else {
                print_message("Invalid package format", RED);
            }
        }
    }

    if (!found) {
        print_message("Package not found: " + package_name, RED);
        return;
    }

    std::string package_url = CPK_REPO_URL + "/" + url_encode(package);
    std::string package_path = CPK_HOME_DIR + "/" + package;
    std::string package_source = CPK_HOME_DIR + "/" + pkgname + "/" + pkgver;

    // Fetch package and its source
    if (!fs::is_directory(package_source)) {
        if (download_file(package_url, package_path)) {
            if (extract_package(package_path, CPK_HOME_DIR)) {
                print_message("Package source extracted successfuly", NONE);
            } else {
                print_message("Failed to extract package info: " + package, RED);
                return;
            }
        }
        else {
            print_message("Failed to download package: " + package, RED);
            return;
        }
    }

    std::string pkgfile_path = package_source + "/Pkgfile";
    std::string package_file;

    // Read Pkgfile
    if (!fs::exists(pkgfile_path)) {
        print_message("Pkgfile not found in " + pkgfile_path, RED);
    } else {
        if (!parse_pkgfile(pkgfile_path, pkgname, pkgdesc, pkgurl, pkgdeps)) {
            print_message("Failed to parse Pkgfile", RED);
        } else {

            package_file = find_pkg_file(package_source, pkgname, pkgver);

            if (fs::exists(package_file)) {
                bool install_failed = false;

                // pre-install
                std::string pkg_preinstall_path = package_source + "/pre-install";
                if (fs::exists(pkg_preinstall_path)) {
                    print_message("Running pre-install", BLUE);
                    std::string preinstall_cmd = "sh -x";
                    std::vector<std::string> preinstall_args = { pkg_preinstall_path };
                    std::string preinstall_output;
                    install_failed = shellcmd(preinstall_cmd, preinstall_args, &preinstall_output);
                    if (install_failed) {
                        print_message("Failed to run pre-install", RED);
                    }
                }

                // pkgadd   
                print_message("Installing " + package, BLUE);
                std::string pkgadd_cmd = "pkgadd";
                std::vector<std::string> pkgadd_args = { package_file };
                std::string pkgadd_output;
                install_failed = shellcmd(pkgadd_cmd, pkgadd_args, &pkgadd_output);
                if (install_failed) {
                    print_message("Failed to run pkgadd", RED);
                }

                // post-install
                std::string pkg_postinstall_path = package_source + "/post-install";
                if (fs::exists(pkg_postinstall_path)) {
                    print_message("Running post-install", BLUE);
                    std::string postinstall_cmd = "sh -x";
                    std::vector<std::string> postinstall_args = { pkg_postinstall_path };
                    std::string postinstall_output;
                    install_failed = shellcmd(postinstall_cmd, postinstall_args, &postinstall_output);
                    if (install_failed) {
                        print_message("Failed to run post-install", RED);
                    }
                }

                // README
                std::string pkg_readme_path = package_source + "/README";
                if (fs::exists(pkg_readme_path)) {
                    print_message("Printing README", BLUE);
                    std::string readme_cmd = "cat";
                    std::vector<std::string> readme_args = { pkg_readme_path };
                    std::string readme_output;
                    install_failed = shellcmd(readme_cmd, readme_args, &readme_output);
                    if (install_failed) {
                        print_message("Failed to print README", RED);
                    }
                }

                if (install_failed) {
                    print_message("Failed to install package", RED);
                } else {
                    print_message("Package installed successfuly", GREEN);
                }
            }
        }
    }
}

// Function to uninstall a package
void cmd_uninstall(const std::vector<std::string>& args) {
    bool uninstall_failed = false;
    std::string pkgrm_cmd = "pkgrm";
    std::vector<std::string> pkgrm_args = {};
    std::string pkgrm_output;
    uninstall_failed = shellcmd(pkgrm_cmd, pkgrm_args, &pkgrm_output);
    if (uninstall_failed) {
        print_message("Failed to uninstall package", RED);
    }
}

// Function to list installed packages
void cmd_list(const std::vector<std::string>& args) {
    bool list_failed = false;
    std::string pkginfo_cmd = "pkginfo";
    std::vector<std::string> pkginfo_args = { "-i" };
    std::string pkginfo_output;
    list_failed = shellcmd(pkginfo_cmd, pkginfo_args, &pkginfo_output);
    if (list_failed) {
        print_message("Failed to list installed packages", RED);
    }
}

// Function to upgrade installed packages
void cmd_upgrade(const std::vector<std::string>& args) {
    print_message("Not implemented yet.", YELLOW);
}

// Function to build a package using pkgmk
void cmd_build(const std::vector<std::string>& args) {
    print_message("Not implemented yet.", YELLOW);
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
}
