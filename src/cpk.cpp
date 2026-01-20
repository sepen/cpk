#include "cpk.h"
#include "utils.h"
#include <fstream>
#include <iostream>
#include <filesystem>
#include <set>
#include <sstream>
#include <system_error>

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

    // Download .cpk.info file instead of .cpk
    std::string info_filename = package + ".info";
    std::string info_url = CPK_REPO_URL + "/" + url_encode(info_filename);

    // Determine where to store the info file
    std::string info_path;
    bool use_temp = false;
    fs::path temp_dir;
    fs::path temp_file;

    // Check if CPK_HOME_DIR is writable
    fs::path home_dir_path(CPK_HOME_DIR);
    bool home_writable = false;
    if (fs::exists(home_dir_path)) {
        // Check if directory is writable
        std::error_code ec;
        fs::path test_file = home_dir_path / ".cpk_write_test";
        std::ofstream test_stream(test_file);
        if (test_stream.is_open()) {
            test_stream.close();
            fs::remove(test_file, ec);
            home_writable = true;
        }
    }

    if (home_writable) {
        info_path = CPK_HOME_DIR + "/" + info_filename;
    } else {
        // Use temporary directory
        use_temp = true;
        temp_dir = fs::temp_directory_path();
        temp_file = temp_dir / ("cpk_" + info_filename);
        info_path = temp_file.string();
        
        if (CPK_VERBOSE) {
            print_message("Using temporary directory: " + temp_dir.string());
        }
    }

    if (!fs::exists(info_path)) {
        if (!download_file(info_url, info_path, false)) {
            print_message("Failed to retrieve package info", RED);
            return;
        }
    }

    // Parse .cpk.info file
    std::string name, version, arch, description, url, dependencies;
    if (!parse_cpk_info(info_path, name, version, arch, description, url, dependencies)) {
        print_message("Failed to parse .cpk.info file", RED);
        // Clean up temp file if used
        if (use_temp && fs::exists(temp_file)) {
            fs::remove(temp_file);
        }
        return;
    }

    print_message(BOLD + "Name         " + NONE + "| " + name);
    print_message(BOLD + "Version      " + NONE + "| " + version);
    print_message(BOLD + "Arch         " + NONE + "| " + arch);
    print_message(BOLD + "Description  " + NONE + "| " + description);
    print_message(BOLD + "URL          " + NONE + "| " + url);
    print_message(BOLD + "Dependencies " + NONE + "| " + dependencies);

    // Clean up temp file if used (optional - OS will clean it up eventually)
    if (use_temp && fs::exists(temp_file)) {
        std::error_code ec;
        fs::remove(temp_file, ec);
    }
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

    std::string upgrade_flag;
    if (is_package_installed(pkgname)) {
        if (args[1] == "--upgrade") {
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

    // We need to download and extract the package if not already done
    // That way we can run pre/post install scripts and show README
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
// Function to upgrade installed packages
void cmd_upgrade(const std::vector<std::string>& args) {

    std::vector<std::string> packages;

    // If no specific packages are provided, upgrade only packages with newer versions available
    if (args.empty()) {
        // Get installed packages
        std::vector<std::string> pkginfo_args = { "-i" };
        std::string installed_packages;
        shellcmd(CPK_PKGINFO_CMD, pkginfo_args, &installed_packages, false);

        // Read installed packages line by line and split into name & version
        std::string installed_pkgname, installed_pkgver;
        std::string package, pkgname, pkgver, pkgarch;
        std::istringstream installed_stream(installed_packages);

        while (installed_stream >> installed_pkgname >> installed_pkgver) {
            if (find_package(installed_pkgname, package, pkgname, pkgver, pkgarch)) {
                if (installed_pkgname == pkgname && compare_versions(installed_pkgver, pkgver) < 0) {
                    // Only add if available version is newer than installed
                    packages.push_back(installed_pkgname);
                }
            }
        }

        if (packages.empty()) {
            print_message("No packages with newer versions available", GREEN);
            return;
        }
    }
    else {
        packages = args;
    }

    // Check which packages have updates available and upgrade them
    for (const std::string& pkg : packages) {
        std::string package, pkgname, pkgver, pkgarch;
        if (find_package(pkg, package, pkgname, pkgver, pkgarch)) {
            if (is_package_installed(pkgname)) {
                // Get installed version to compare
                std::vector<std::string> pkginfo_args = { "-i" };
                std::string installed_packages;
                shellcmd(CPK_PKGINFO_CMD, pkginfo_args, &installed_packages, false);

                std::istringstream stream(installed_packages);
                std::string line;
                std::string installed_version;

                while (std::getline(stream, line)) {
                    if (line.find(pkgname) != std::string::npos) {
                        std::istringstream line_stream(line);
                        std::string temp_name, temp_version;
                        line_stream >> temp_name >> temp_version;
                        if (temp_name == pkgname) {
                            installed_version = temp_version;
                            break;
                        }
                    }
                }

                // Only upgrade if there's a newer version
                if (!installed_version.empty() && compare_versions(installed_version, pkgver) < 0) {
                    cmd_install({pkgname, "--upgrade"});
                }
                else if (!installed_version.empty()) {
                    print_message("Package " + pkgname + " is already up to date", YELLOW);
                }
            }
            else {
                print_message("Package " + pkgname + " is not installed", YELLOW);
            }
        }
        else {
            print_message("Package " + pkg + " not found in index", RED);
        }
    }
 
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

            // Get port source directory and read Pkgfile
            std::string package_prefix = package.substr(0, package.find(".pkg."));
            fs::path package_dir = package_path.parent_path();
            fs::path pkgfile_path = package_dir / "Pkgfile";
            std::ifstream pkgfile(pkgfile_path);
            if (!pkgfile) continue;

            // Parse Pkgfile once to get all needed values
            std::string name, version, release;
            std::string pkgdesc, pkgurl, pkgdeps;
            std::vector<std::string> sources;
            std::string line;
            while (std::getline(pkgfile, line)) {
                // Trim leading/trailing whitespace
                std::string trimmed_line = line;
                trimmed_line.erase(0, trimmed_line.find_first_not_of(" \t"));
                trimmed_line.erase(trimmed_line.find_last_not_of(" \t") + 1);

                if (trimmed_line.find("name=") == 0) {
                    name = trimmed_line.substr(5);
                } else if (trimmed_line.find("version=") == 0) {
                    version = trimmed_line.substr(8);
                } else if (trimmed_line.find("release=") == 0) {
                    release = trimmed_line.substr(8);
                } else if (trimmed_line.find("source=(") == 0) {
                    std::string source_line = trimmed_line.substr(7, trimmed_line.length() - 8); // Remove 'source=(' and ')'
                    sources.push_back(source_line);
                } else if (trimmed_line.find("# Description:") == 0) {
                    pkgdesc = trimmed_line.substr(14); // Extract after '# Description:'
                } else if (trimmed_line.find("# URL:") == 0) {
                    pkgurl = trimmed_line.substr(6); // Extract after '# URL:'
                } else if (trimmed_line.find("# Depends on:") == 0) {
                    pkgdeps = trimmed_line.substr(13); // Extract after '# Depends on:'
                }
            }
            pkgfile.close();

            if (package_prefix == name + "#" + version + "-" + release) {
                std::string cpk_filename = package_prefix + "." + arch + ".cpk";
                fs::path cpk_path = output_dir / cpk_filename;

                // Check if .cpk file already exists
                if (fs::exists(cpk_path)) {
                    if (CPK_VERBOSE) {
                        print_message("Skipping " + cpk_path.string() + " (already exists)");
                    }
                } else {
                    print_message("Packaging " + output_dir.string() + "/" + cpk_filename);
                    fs::path basedir = output_dir / name / (version + "-" + release);
                    ensure_directory(basedir);
                    auto local_files = get_local_files(sources);
                    copy_files(package_dir, basedir, local_files);
                    fs::copy(package_path, basedir / package, fs::copy_options::overwrite_existing);
                    package_files(name, version, release, arch, output_dir);
                }

                // Generate .cpk.info file (only if .cpk file exists)
                fs::path info_path = output_dir / (cpk_filename + ".info");
                if (!fs::exists(info_path) && fs::exists(cpk_path)) {
                    // Calculate SHA256 checksum of .cpk file
                    std::string checksum = calculate_sha256(cpk_path.string());

                    if (!checksum.empty()) {
                        // Write .cpk.info file
                        std::ofstream info_file(info_path);
                        if (info_file.is_open()) {
                            info_file << "checksum: " << checksum << "\n";
                            info_file << "name: " << name << "\n";
                            info_file << "version: " << version << "-" << release << "\n";
                            info_file << "arch: " << arch << "\n";
                            info_file << "description: " << ltrim(pkgdesc) << "\n";
                            info_file << "url: " << ltrim(pkgurl) << "\n";
                            info_file << "dependencies: " << ltrim(pkgdeps) << "\n";
                            info_file.close();

                            if (CPK_VERBOSE) {
                                print_message("Generated " + info_path.string());
                            }
                        } else {
                            print_message("Failed to create .cpk.info file: " + info_path.string(), RED);
                        }
                    } else {
                        print_message("Failed to calculate checksum for " + cpk_path.string(), RED);
                    }
                } else if (fs::exists(info_path)) {
                    if (CPK_VERBOSE) {
                        print_message("Skipping " + info_path.string() + " (already exists)");
                    }
                }
            }
        }
    }
    return;
}