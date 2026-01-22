#include "cpk.h"
#include "utils.h"
#include <string>
#include <vector>
#include <filesystem>
#include <algorithm>
#include <archive.h>
#include <archive_entry.h>
#include <curl/curl.h>
#include <sys/stat.h>
#include <iomanip>
#include <cstdlib>
#include <stdexcept>
#include <regex>
#include <fstream>
#include <sstream>
#include <iostream>

namespace fs = std::filesystem;

const std::string RED     = "\033[31m";
const std::string GREEN   = "\033[32m";
const std::string BLUE    = "\033[34m"; // header
const std::string YELLOW  = "\033[33m"; // not success
const std::string BOLD    = "\033[1m";
const std::string NONE    = "\033[0m";
const std::string NEWLINE = "\n";

// Helper function to remove left leading spaces and tabs from a string
std::string ltrim(const std::string& str) {
    std::string s = str;
    size_t first = s.find_first_not_of(" \t");
    if (first != std::string::npos) {
        s.erase(0, first);
    } else {
        s.clear(); // String is all whitespace
    }
    return s;
}

// Helper function to write the downloaded content to a file
static size_t write_data(void *ptr, size_t size, size_t nmemb, FILE *stream) {
    size_t written = fwrite(ptr, size, nmemb, stream);
    return written;
}

// Function to compare versions semantically
// Returns: -1 if v1 < v2, 0 if v1 == v2, 1 if v1 > v2
// Only compares the VERSION part (before the first '-'), ignoring RELEASE
int compare_versions(const std::string& v1, const std::string& v2) {
    std::vector<int> ver1, ver2;
    
    // Extract version part (before '-') if present and parse v1
    size_t dash1 = v1.find('-');
    std::string ver1_str = (dash1 != std::string::npos) ? v1.substr(0, dash1) : v1;
    std::istringstream iss1(ver1_str);
    std::string token;
    while (std::getline(iss1, token, '.')) {
        try {
            ver1.push_back(std::stoi(token));
        } catch (const std::exception& e) {
            ver1.push_back(0);
        }
    }
    
    // Extract version part (before '-') if present and parse v2
    size_t dash2 = v2.find('-');
    std::string ver2_str = (dash2 != std::string::npos) ? v2.substr(0, dash2) : v2;
    std::istringstream iss2(ver2_str);
    while (std::getline(iss2, token, '.')) {
        try {
            ver2.push_back(std::stoi(token));
        } catch (const std::exception& e) {
            ver2.push_back(0);
        }
    }
    
    // Compare element by element
    size_t max_len = std::max(ver1.size(), ver2.size());
    for (size_t i = 0; i < max_len; i++) {
        int v1_part = (i < ver1.size()) ? ver1[i] : 0;
        int v2_part = (i < ver2.size()) ? ver2[i] : 0;
        
        if (v1_part < v2_part) return -1;
        if (v1_part > v2_part) return 1;
    }
    
    return 0; // They are equal
}

// URL-encodes special characters in the given string
std::string url_encode(const std::string &value) {
    std::ostringstream encoded;
    for (unsigned char c : value) {
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            encoded << c; // Leave unreserved characters as is
        } else {
            encoded << '%' << std::uppercase << std::hex << std::setw(2) << std::setfill('0') << int(c);
        }
    }
    return encoded.str();
}

// URL-decodes special characters in the given string
std::string url_decode(const std::string &value) {
    std::ostringstream decoded;
    for (size_t i = 0; i < value.length(); i++) {
        if (value[i] == '%' && i + 2 < value.length()) {
            std::istringstream hex_stream(value.substr(i + 1, 2));
            int hex_value;
            if (hex_stream >> std::hex >> hex_value) {
                decoded << static_cast<char>(hex_value);
                i += 2; // Skip next two hex characters
            }
        } else if (value[i] == '+') {
            decoded << ' '; // Convert '+' to space
        } else {
            decoded << value[i]; // Normal character
        }
    }
    return decoded.str();
}

// Prompt user for confirmation
bool prompt_user(const std::string& message) {
    std::string response;
    std::cout << message << " [y/N]: ";
    std::getline(std::cin, response);
    return (response == "y" || response == "Y");
}

// Function to download a file from a URL
bool download_file(const std::string &url, const std::string &file_path, bool overwrite) {

    if (fs::exists(file_path) && !overwrite) {
        return true;
    }

    if (CPK_VERBOSE) print_message("Fetching " + url_decode(url));
    
    FILE *fp = fopen(file_path.c_str(), "wb");
    if (fp == nullptr) {
        print_message("Failed to file for writting " + file_path, RED);
        return false;
    }

    CURL *curl = curl_easy_init();
    if (!curl) {
        print_message("Failed to initialize CURL", RED);
        fclose(fp);
        return false;
    }

    // Set CURL options
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L); // Follow redirects if any

    CURLcode res = curl_easy_perform(curl);
    
    long http_code = 0;
    if (res == CURLE_OK) {
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
    }
    
    // Clean up
    curl_easy_cleanup(curl);
    fclose(fp);

    if (res != CURLE_OK) {
        print_message("Download error: " + std::string(curl_easy_strerror(res)), RED);
        fs::remove(file_path); // Remove partial file
        return false;
    }
    
    // Check HTTP response code
    if (http_code >= 400) {
        if (CPK_VERBOSE) {
            print_message("HTTP error " + std::to_string(http_code) + " for " + url_decode(url), RED);
        }
        fs::remove(file_path); // Remove error response file
        return false;
    }

    return true;
}

// Function to print colored header (if enabled in config)
void print_header(const std::string& message, const std::string& color) {
    if (CPK_COLOR_MODE) {
        std::cout << color << "==> \033[0m\033[1m" << message << "\033[0m" << std::endl;
    } else {
        std::cout << "==> " << message << std::endl;
    }
    return;
}

// Function to print colored messages (if enabled in config)
void print_message(const std::string& message, const std::string& color) {
    if (CPK_COLOR_MODE) {
        std::cout << color << message << "\033[0m" << std::endl;
    } else {
        std::cout << message << std::endl;
    }
    return;
}

// Function to show version information
void print_version() {
    print_message("cpk " + CPK_VERSION);
}

// Display general options
void print_general_options() {
    print_message("\nGeneral Options:");
    print_message("  -c, --config <file>      Set an alternative configuration file (default: /etc/cpk.conf)");
    print_message("  -r, --root <path>        Set an alternative installation root (default: /)");
    print_message("  -C, --color              Show colorized output messages");
    print_message("  -v, --verbose            Show verbose output messages");
    print_message("  -h, --help               Print this help information");
}

// Display help for specific commands
void print_help_info() {
    print_message("Usage: cpk info <package> [--field]");
    print_message("\nDescription:");
    print_message("  Show information about installed or available packages");
    print_message("\nArguments:");
    print_message("  <package>                Package name");
    print_message("\nFields:");
    print_message("  --name                   Show package name only");
    print_message("  --version                Show package version only");
    print_message("  --arch                   Show package architecture only");
    print_message("  --description            Show package description only");
    print_message("  --url                    Show package URL only");
    print_message("  --dependencies           Show package dependencies only");
    print_message("\nExamples:");
    print_message("  cpk info vim");
    print_message("  cpk info vim --version");
    print_message("  cpk info vim --dependencies");
    print_general_options();
}

void print_help_install() {
    print_message("Usage: cpk install <package> [--upgrade]");
    print_message("       cpk add <package> [--upgrade]");
    print_message("\nDescription:");
    print_message("  Install or upgrade packages on the system");
    print_message("\nArguments:");
    print_message("  <package>                Package name");
    print_message("  --upgrade                Upgrade package if already installed");
    print_message("\nAliases:");
    print_message("  add                      Alias for install");
    print_message("\nExamples:");
    print_message("  cpk install vim");
    print_message("  cpk add vim --upgrade");
    print_general_options();
}

void print_help_uninstall() {
    print_message("Usage: cpk uninstall <package>");
    print_message("       cpk del <package>");
    print_message("       cpk rm <package>");
    print_message("\nDescription:");
    print_message("  Remove packages from the system");
    print_message("\nArguments:");
    print_message("  <package>                Package name");
    print_message("\nAliases:");
    print_message("  del, rm                  Aliases for uninstall");
    print_message("\nExamples:");
    print_message("  cpk uninstall vim");
    print_message("  cpk rm vim");
    print_general_options();
}

void print_help_update() {
    print_message("Usage: cpk update");
    print_message("\nDescription:");
    print_message("  Update the index of available packages from the repository");
    print_message("\nExamples:");
    print_message("  cpk update");
    print_general_options();
}

void print_help_deps() {
    print_message("Usage: cpk deps <package>");
    print_message("\nDescription:");
    print_message("  Show package dependencies (alias for 'cpk info --dependencies')");
    print_message("\nArguments:");
    print_message("  <package>                Package name");
    print_message("\nExamples:");
    print_message("  cpk deps vim");
    print_general_options();
}

void print_help_search() {
    print_message("Usage: cpk search <keyword>");
    print_message("\nDescription:");
    print_message("  Search for packages by name or keyword in the package index");
    print_message("\nArguments:");
    print_message("  <keyword>                Search term");
    print_message("\nExamples:");
    print_message("  cpk search vim");
    print_message("  cpk search editor");
    print_general_options();
}

void print_help_list() {
    print_message("Usage: cpk list");
    print_message("\nDescription:");
    print_message("  List all installed packages");
    print_message("\nExamples:");
    print_message("  cpk list");
    print_general_options();
}

void print_help_diff() {
    print_message("Usage: cpk diff");
    print_message("\nDescription:");
    print_message("  Show differences between installed and available packages");
    print_message("\nExamples:");
    print_message("  cpk diff");
    print_general_options();
}

void print_help_verify() {
    print_message("Usage: cpk verify <package>");
    print_message("\nDescription:");
    print_message("  Verify integrity of package source files using signatures");
    print_message("\nArguments:");
    print_message("  <package>                Package name");
    print_message("\nExamples:");
    print_message("  cpk verify vim");
    print_general_options();
}

void print_help_build() {
    print_message("Usage: cpk build <package>");
    print_message("\nDescription:");
    print_message("  Build a package from source files using pkgmk");
    print_message("\nArguments:");
    print_message("  <package>                Package name");
    print_message("\nExamples:");
    print_message("  cpk build vim");
    print_general_options();
}

void print_help_upgrade() {
    print_message("Usage: cpk upgrade [<package>...]");
    print_message("\nDescription:");
    print_message("  Upgrade all installed packages to the latest versions");
    print_message("  If package names are provided, only upgrade those packages");
    print_message("\nArguments:");
    print_message("  <package>                Optional: specific package(s) to upgrade");
    print_message("\nExamples:");
    print_message("  cpk upgrade");
    print_message("  cpk upgrade vim busybox");
    print_general_options();
}

void print_help_clean() {
    print_message("Usage: cpk clean");
    print_message("\nDescription:");
    print_message("  Clean up package source files and temporary directories");
    print_message("  Removes all cached packages except CPKINDEX");
    print_message("\nExamples:");
    print_message("  cpk clean");
    print_general_options();
}

void print_help_index() {
    print_message("Usage: cpk index <repo>");
    print_message("\nDescription:");
    print_message("  Create a CPKINDEX file for a local repository");
    print_message("\nArguments:");
    print_message("  <repo>                   Path to repository directory");
    print_message("\nExamples:");
    print_message("  cpk index /path/to/repo");
    print_general_options();
}

void print_help_archive() {
    print_message("Usage: cpk archive <prtdir> <repo>");
    print_message("\nDescription:");
    print_message("  Create .cpk archive(s) from a directory containing ports");
    print_message("\nArguments:");
    print_message("  <prtdir>                 Path to ports directory");
    print_message("  <repo>                   Path to output repository directory");
    print_message("\nExamples:");
    print_message("  cpk archive /usr/ports /var/cpk/repo");
    print_general_options();
}

// Display the help message
void print_help(const std::string& command) {
    if (command.empty()) {
        // Basic help view
        print_message("CRUX Package Keeper - package management tool for CRUX Linux");

        print_message("\nUsage:");
        print_message("  cpk <command> [options]");
        print_message("  cpk help <command>   Show detailed help for a specific command");

        print_message("\nCommands:");
        print_message("  update      Update the index of available packages");
        print_message("  info        Show information about installed or available packages");
        print_message("  deps        Show package dependencies");
        print_message("  search      Search for packages by name or keyword");
        print_message("  list        List all installed packages");
        print_message("  diff        Show differences between installed and available packages");
        print_message("  verify      Verify integrity of package source files");
        print_message("  build       Build a package from source files");
        print_message("  install     Install or upgrade packages on the system");
        print_message("  add         Alias for install");
        print_message("  uninstall   Remove packages from the system");
        print_message("  del         Alias for uninstall");
        print_message("  rm          Alias for uninstall");
        print_message("  upgrade     Upgrade all installed packages to the latest versions");
        print_message("  clean       Clean up package source files and temporary directories");
        print_message("  index       Create a CPKINDEX file for a local repository");
        print_message("  archive     Create .cpk archive(s) from a directory containing ports");
        print_message("  help        Show this help message or detailed help for a command");
        print_message("  version     Show version information");

        print_general_options();
    } else {
        // Command-specific help
        if (command == "info") {
            print_help_info();
        } else if (command == "install" || command == "add") {
            print_help_install();
        } else if (command == "uninstall" || command == "del" || command == "rm") {
            print_help_uninstall();
        } else if (command == "update") {
            print_help_update();
        } else if (command == "deps") {
            print_help_deps();
        } else if (command == "search") {
            print_help_search();
        } else if (command == "list") {
            print_help_list();
        } else if (command == "diff") {
            print_help_diff();
        } else if (command == "verify") {
            print_help_verify();
        } else if (command == "build") {
            print_help_build();
        } else if (command == "upgrade") {
            print_help_upgrade();
        } else if (command == "clean") {
            print_help_clean();
        } else if (command == "index") {
            print_help_index();
        } else if (command == "archive") {
            print_help_archive();
        } else {
            print_message("Unknown command: " + command);
            print_message("Run 'cpk help' to see available commands");
            print_general_options();
        }
    }
}

// Function to extract tarball
bool extract_package(const std::string& tar_file, const std::string& dest_dir) {
    struct archive* a = archive_read_new();  // Create archive object
    struct archive_entry* entry;

    // Register the format as tar-based
    archive_read_support_format_tar(a);

    // Open the archive
    if (archive_read_open_filename(a, tar_file.c_str(), 10240) != ARCHIVE_OK) {
        if (CPK_VERBOSE > 0) {
            std::cerr << "Error opening archive: " << archive_error_string(a) << std::endl;
        }
        archive_read_free(a);
        return false;
    }

    // Read and extract entries
    while (archive_read_next_header(a, &entry) == ARCHIVE_OK) {
        const char* entry_name = archive_entry_pathname(entry);
        if (CPK_VERBOSE > 0) {
            std::cout << "Extracting: " << entry_name << std::endl;
        }

        // Modify the entry's path to include the destination directory
        std::string dest_path = dest_dir + "/" + entry_name;
        archive_entry_set_pathname(entry, dest_path.c_str()); // Set the destination path for the entry

        // Extract with the ARCHIVE_EXTRACT_TIME flag
        if (archive_read_extract(a, entry, ARCHIVE_EXTRACT_TIME) != ARCHIVE_OK) {
            if (CPK_VERBOSE > 0) {
                std::cerr << "Error extracting file: " << archive_error_string(a) << std::endl;
            }
            archive_read_free(a);
            return false;
        }
    }

    archive_read_free(a);  // Clean up
    return true;
}

// Function to parse the Pkgfile
bool parse_pkgfile(const std::string& pkgfile_path, std::string& name, std::string& desc, std::string& url, std::string& deps) {
    std::ifstream infile(pkgfile_path);
    if (!infile.is_open()) {
        return false;
    }

    std::string line;
    while (std::getline(infile, line)) {
        // Trim leading/trailing whitespace
        line.erase(0, line.find_first_not_of(" \t"));
        line.erase(line.find_last_not_of(" \t") + 1);

        if (line.rfind("name=", 0) == 0) {
            name = ltrim(line.substr(5)); // Extract after 'name=' and trim
        } else if (line.rfind("# Description:", 0) == 0) {
            desc = ltrim(line.substr(14)); // Extract after '# Description:' and trim
        } else if (line.rfind("# URL:", 0) == 0) {
            url = ltrim(line.substr(6)); // Extract after '# URL:' and trim
        } else if (line.rfind("# Depends on:", 0) == 0) {
            deps = ltrim(line.substr(13)); // Extract after '# Depends on:' and trim
        }
    }

    return true;
}

// Function to load the configuration from the cpk.conf file
bool load_cpk_config(const std::string& config_file) {
    std::ifstream file(config_file);
    if (!file.is_open()) {
        print_message("Error opening config file " + config_file, RED);
        return false;
    }

    std::string line;
    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string key;
        iss >> key;

        if (key == "cpk_repo_url") {
            iss >> CPK_REPO_URL;
        } else if (key == "cpk_home_dir") {
            iss >> CPK_HOME_DIR;
        } else if (key == "cpk_install_root") {
            iss >> CPK_INSTALL_ROOT;
        } else if (key == "cpk_pkgmk_cmd") {
            iss >> CPK_PKGMK_CMD;
        } else if (key == "cpk_pkgadd_cmd") {
            iss >> CPK_PKGADD_CMD;
        } else if (key == "cpk_pkgrm_cmd") {
            iss >> CPK_PKGRM_CMD;
        } else if (key == "cpk_pkginfo_cmd") {
            iss >> CPK_PKGINFO_CMD;
        } else if (key == "cpk_color_mode") {
            std::string colors;
            iss >> colors;
            CPK_COLOR_MODE = (colors == "true");
        } else if (key == "cpk_verbose") {
            std::string verbose;
            iss >> verbose;
            CPK_VERBOSE = ( verbose == "true");
        } 
    }

    file.close();
    return true;
}

// Function to execute a shell command and output lines in real-time
int shellcmd(const std::string& command, const std::vector<std::string>& args, std::string* output, bool show_output) {
    std::string full_command = command;
    for (const std::string& arg : args) {
        full_command += " " + arg;
    }
    // Redirect stderr to stdout so both are captured
    full_command += " 2>&1";

    char buffer[128];
    if (output) output->clear();  // Clear the output if provided

    FILE* pipe = popen(full_command.c_str(), "r"); // Start the command
    if (!pipe) {
        print_message("Error executing shell command: " + full_command, RED);
        return -1; // Indicate failure to execute the command
    }

    // Read and output each line as it's received
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        std::string line(buffer);
        if (show_output) {
            std::cout << line; // Real-time output
        }
        if (output) (*output) += line; // Store the result if requested
    }

    int status = pclose(pipe); // Get the command's exit status
    int return_code = -1;      // Default error code if abnormal termination

    if (WIFEXITED(status)) {
        return_code = WEXITSTATUS(status);
    }

    return return_code;
}

// Helper function to run scripts
bool run_script(const std::string& script_path, const std::string& msg) {
    if (!fs::exists(script_path)) {
        return false;
    }
    // pre-install and post-install scripts do not support alternative installation root
    if (CPK_INSTALL_ROOT != "/") {
        print_message("Skipping script " + script_path + " (not supported with alternative root)", YELLOW);
        return true;
    }
    print_header(msg, GREEN);
    if (shellcmd("sh", {"-x", script_path}, nullptr) != 0) {
        print_message("Failed to execute script " + script_path, RED);
        return false;
    }
    return true;
}

// Function to find compression mode and the full path for package file
std::string find_pkg_file(const std::string& directory, const std::string& pkgname, const std::string& pkgver) {
    std::string matched_file;
    std::vector<std::string> compression_modes = {"gz", "bz2", "xz"};

    for (const auto& compression_mode : compression_modes) {
        std::string full_path = directory + "/" + pkgname + "#" + pkgver + ".pkg.tar." + compression_mode;
        if (fs::exists(full_path)) {
            matched_file = fs::path(full_path).string();
        }
    }

    return matched_file;
}

// Helper function to find package details
bool find_package(const std::string& package_name, std::string& package, std::string& pkgname, std::string& pkgver, std::string& pkgarch) {
    // Try cache directory first, then fallback to CPK_HOME_DIR
    std::string index_file = get_cache_file("CPKINDEX");
    if (!fs::exists(index_file)) {
        index_file = CPK_HOME_DIR + "/CPKINDEX";
    }
    
    bool result = false;
    std::string best_version;  // Store the newest version

    if (!fs::exists(index_file)) {
        print_message("Package index not found. Run `cpk update` first", RED);
        result = false;
    } else {
        std::ifstream file(index_file);
        if (!file.is_open()) {
            print_message("Error opening file: " + index_file, RED);
            result = false;
        }
        else {
            std::string index_line;
            while (std::getline(file, index_line)) {
                if (index_line.find(package_name + "#") != std::string::npos) {
                    size_t cpk_pos = index_line.rfind(".cpk");
                    size_t last_dot = index_line.rfind('.', cpk_pos - 1);
                    size_t hash_pos = index_line.find('#');

                    if (cpk_pos != std::string::npos && last_dot != std::string::npos && hash_pos != std::string::npos) {
                        std::string temp_pkgname = index_line.substr(0, hash_pos);
                        std::string temp_pkgver = index_line.substr(hash_pos + 1, last_dot - hash_pos - 1);
                        std::string temp_pkgarch = index_line.substr(last_dot + 1, cpk_pos - last_dot - 1);
                        
                        if (package_name == temp_pkgname) {
                            // If it's the first version or newer than the previous one
                            if (!result || compare_versions(temp_pkgver, best_version) > 0) {
                                pkgname = temp_pkgname;
                                pkgver = temp_pkgver;
                                pkgarch = temp_pkgarch;
                                package = index_line;
                                best_version = temp_pkgver;
                                result = true;
                            }
                        }
                    }
                }
            }
        }
        file.close();
    }

    return result;
}

bool is_package_installed(const std::string& package_name) {
    std::string pkginfo_cmd = "pkginfo";
    std::vector<std::string> pkginfo_args = { "-i" };
    std::string pkginfo_output;

    shellcmd(pkginfo_cmd, pkginfo_args, &pkginfo_output, false);

    std::istringstream stream(pkginfo_output);
    std::string line;

    while (std::getline(stream, line)) {
        if (line.find(package_name) != std::string::npos) {
            return true;  // Package found
        }
    }

    return false;  // Package not found
}

int get_number_of_packages() {
    // Try cache directory first, then fallback to CPK_HOME_DIR
    std::string index_file = get_cache_file("CPKINDEX");
    if (!fs::exists(index_file)) {
        index_file = CPK_HOME_DIR + "/CPKINDEX";
    }
    if (!fs::exists(index_file)) {
        print_message("Package index not found. Run `cpk update` first", RED);
        return false;
    }

    std::ifstream file(index_file);  // Open the CPKINDEX file for reading
    if (!file.is_open()) {
        print_message("Error opening file: " + index_file, RED);
        return -1;  // Return an error code if the file cannot be opened
    }

    int package_count = 0;
    std::string index_line;
    
    // Read the file line by line and count the lines (packages)
    while (std::getline(file, index_line)) {
        if (!index_line.empty()) {  // Count only non-empty lines
            ++package_count;  // Increment for each non-empty line
        }
    }

    file.close();  // Close the file
    return package_count;
}

bool change_directory(const std::string& path) {
    return chdir(path.c_str()) == 0;
}

// Function to print formatted header with columns
void print_fmt_header(const std::string& header_text) {
    std::istringstream iss(header_text);
    std::string col1, col2, col3;

    iss >> col1 >> col2 >> col3;

    std::cout << BOLD
              << std::left << std::setw(40) << col1
              << std::setw(20) << (col2.empty() ? "" : col2)
              << std::setw(20) << (col3.empty() ? "" : col3)
              << NONE << std::endl;

    std::cout << std::left
              << std::setw(40) << std::string(col1.size(), '-')
              << std::setw(20) << (col2.empty() ? "" : std::string(col2.size(), '-'))
              << std::setw(20) << (col3.empty() ? "" : std::string(col3.size(), '-'))
              << std::endl;
}

// Function to print lines in formatted columns
void print_fmt_lines(const std::string& text) {
    std::string line;
    std::istringstream line_stream(text);

    while (std::getline(line_stream, line)) {
        std::istringstream columns(line);
        std::string col1, col2, col3;

        columns >> col1 >> col2 >> col3;
        std::cout << std::left
                  << std::setw(40) << col1
                  << std::setw(20) << col2
                  << std::setw(20) << col3
                  << std::endl;
    }
}

// Function to find all `.pub` files in `/etc/ports/`
std::vector<std::string> find_public_keys(const std::string& directory) {
    std::vector<std::string> pub_keys;

    for (const auto& entry : fs::directory_iterator(directory)) {
        if (entry.is_regular_file() && entry.path().extension() == ".pub") {
            pub_keys.push_back(entry.path().string());
        }
    }

    return pub_keys;
}

// Helper function to create a directory if it doesn't exist
void ensure_directory(const fs::path &dir) {
    if (!fs::exists(dir)) {
        fs::create_directories(dir);
    }
}

// Function to get local files from sources list
std::vector<std::string> get_local_files(const std::vector<std::string> &sources) {
    std::vector<std::string> local_files = {"Pkgfile", ".footprint", ".signature", "pre-install", "post-install", "README"};
    std::regex url_regex("^(https?|ftp)://");
    for (const auto &item : sources) {
        if (!std::regex_match(item, url_regex)) {
            local_files.push_back(item);
        }
    }
    return local_files;
}

// Function to copy files from source to destination directory
void copy_files(const fs::path &source_dir, const fs::path &dest_dir, const std::vector<std::string> &files) {
    create_directory(dest_dir);
    for (const auto &file : files) {
        fs::path src = source_dir / file;
        fs::path dest = dest_dir / file;
        if (fs::exists(src)) {
            fs::copy(src, dest, fs::copy_options::overwrite_existing);
        }
    }
}

// Function to package files into a .cpk archive
void package_files(const std::string &name, const std::string &version, const std::string &release, const std::string &arch, const fs::path &output_dir) {
    fs::path package_path = output_dir / (name + "#" + version + "-" + release + "." + arch + ".cpk");

    struct archive *a = archive_write_new();
    archive_write_set_format_pax_restricted(a);
    archive_write_open_filename(a, package_path.c_str());

    fs::path basedir = output_dir / name;
    for (const auto &entry : fs::recursive_directory_iterator(basedir)) {
        if (!entry.is_regular_file()) {
            // Ommit non-regular files (directories, symlinks, etc.)
            continue;
        }

        struct archive_entry *entry_struct = archive_entry_new();

        // Relative path to .cpk file
        fs::path rel_path = fs::relative(entry.path(), output_dir);
        archive_entry_set_pathname(entry_struct, rel_path.c_str());

        // Setup basic file properties
        auto filesize = fs::file_size(entry.path());
        archive_entry_set_size(entry_struct, filesize);
        archive_entry_set_filetype(entry_struct, AE_IFREG);
        archive_entry_set_perm(entry_struct, 0644);

        // Write file header
        archive_write_header(a, entry_struct);

        // Write contents of the file
        std::ifstream file(entry.path(), std::ios::binary);
        std::vector<char> buffer(filesize);
        file.read(buffer.data(), buffer.size());
        archive_write_data(a, buffer.data(), buffer.size());

        archive_entry_free(entry_struct);
    }

    archive_write_close(a);
    archive_write_free(a);

    // Clean up temporary directory
    fs::remove_all(basedir);
}


// Function to update the index of a local repositor
void generate_cpk_index(const fs::path &repo_dir) {
    std::vector<std::string> cpk_files;
    for (const auto &entry : fs::directory_iterator(repo_dir)) {
        if (entry.is_regular_file() && entry.path().extension() == ".cpk") {
            cpk_files.push_back(entry.path().filename().string());
        }
    }

    std::sort(cpk_files.rbegin(), cpk_files.rend());

    fs::path index_tmp = repo_dir / "CPKINDEX.tmp";
    std::ofstream index_file(index_tmp);
    for (const auto &file : cpk_files) {
        index_file << file << "\n";
    }
    index_file.close();

    fs::rename(index_tmp, repo_dir / "CPKINDEX");
}

// Function to get system architecture
std::string get_system_architecture() {
    std::string uname_cmd = "uname";
    std::vector<std::string> uname_args = { "-m" };
    std::string uname_output;

    // Run the command
    if (shellcmd(uname_cmd, uname_args, &uname_output, false) != 0 || uname_output.empty()) {
        print_message("Failed to get system architecture, defaulting to x86_64", YELLOW);
        uname_output = "x86_64";
    }

    // Remove newline characters
    uname_output.erase(std::remove(uname_output.begin(), uname_output.end(), '\n'), uname_output.end());

    // Normalize architecture names
    if (uname_output == "aarch64" || uname_output == "arm64") {
        return "arm64";
    } else if (uname_output == "armv7l" || uname_output == "armv6l") {
        return "armhf";
    } else if (uname_output == "x86_64" || uname_output == "amd64") {
        return "x86_64";
    } else if (uname_output == "i686") {
        return "i686";
    } else {
        print_message("Warning: Unrecognized architecture '" + uname_output + "'", YELLOW);
        return uname_output;
    }
}

// Function to get installed packages
std::vector<std::string> get_installed_packages() {
    std::vector<std::string> installed_packages;
    std::vector<std::string> pkginfo_args = { "-i" };
    std::string pkginfo_output;

    if (shellcmd(CPK_PKGINFO_CMD, pkginfo_args, &pkginfo_output, false) != 0) {
        print_message("Failed to get list of installed packages", RED);
    }
    else {
        std::istringstream stream(pkginfo_output);
        std::string line;

        while (std::getline(stream, line)) {
            std::istringstream line_stream(line);
            std::string pkgname;
            line_stream >> pkgname; // Extract the package name (first column)
            if (!pkgname.empty()) {
                installed_packages.push_back(pkgname);
            }
        }
    }

    return installed_packages;
}

// Function to calculate SHA256 checksum of a file
std::string calculate_sha256(const std::string &file_path) {
    std::string checksum;
    std::vector<std::string> sha256sum_args;

    // Try sha256sum first (Linux), then shasum -a 256 (macOS/BSD)
    std::string sha256sum_cmd = "sha256sum";
    sha256sum_args.push_back(file_path);
    std::string sha256sum_output;

    int ret = shellcmd(sha256sum_cmd, sha256sum_args, &sha256sum_output, false);

    if (ret != 0) {
        // Try shasum -a 256 as fallback
        sha256sum_cmd = "shasum";
        sha256sum_args.clear();
        sha256sum_args.push_back("-a");
        sha256sum_args.push_back("256");
        sha256sum_args.push_back(file_path);
        ret = shellcmd(sha256sum_cmd, sha256sum_args, &sha256sum_output, false);
    }

    if (ret == 0 && !sha256sum_output.empty()) {
        // Extract checksum (first 64 characters before space)
        std::istringstream iss(sha256sum_output);
        iss >> checksum;
    }

    return checksum;
}

// Function to parse a .cpk.info file
bool parse_cpk_info(const std::string &info_file_path, std::string &name, std::string &version, std::string &arch, std::string &description, std::string &url, std::string &dependencies) {
    std::ifstream infile(info_file_path);
    if (!infile.is_open()) {
        return false;
    }

    std::string line;
    bool found_valid_field = false;
    while (std::getline(infile, line)) {
        // Skip empty lines and comments
        if (line.empty() || line[0] == '#') {
            continue;
        }

        // Check if this looks like HTML (common in 404 pages)
        if (line.find("<html") != std::string::npos || line.find("<!DOCTYPE") != std::string::npos) {
            infile.close();
            return false;
        }

        // Find the colon separator
        size_t colon_pos = line.find(':');
        if (colon_pos == std::string::npos) {
            continue;
        }

        std::string key = line.substr(0, colon_pos);
        std::string value = line.substr(colon_pos + 1);

        // Trim whitespace from key and value
        key = ltrim(key);
        size_t key_end = key.find_last_not_of(" \t\r\n");
        if (key_end != std::string::npos) {
            key = key.substr(0, key_end + 1);
        } else {
            key.clear();
        }
        value = ltrim(value);
        size_t value_end = value.find_last_not_of(" \t\r\n");
        if (value_end != std::string::npos) {
            value = value.substr(0, value_end + 1);
        } else {
            value.clear();
        }

        if (key == "name") {
            name = value;
            found_valid_field = true;
        } else if (key == "version") {
            version = value;
            found_valid_field = true;
        } else if (key == "arch") {
            arch = value;
            found_valid_field = true;
        } else if (key == "description") {
            description = value;
            found_valid_field = true;
        } else if (key == "url") {
            url = value;
            found_valid_field = true;
        } else if (key == "dependencies") {
            dependencies = value;
            found_valid_field = true;
        }
    }

    infile.close();
    
    // Return true only if we found at least one valid field (name and version are required)
    return found_valid_field && !name.empty() && !version.empty();
}

// Function to get cache directory, trying CPK_HOME_DIR first, then falling back to ~/.cpk
std::string get_cache_dir() {
    // Check if we can write to CPK_HOME_DIR
    if (fs::exists(CPK_HOME_DIR) && fs::is_directory(CPK_HOME_DIR)) {
        // Try to create a test file to check write permissions
        std::string test_file = CPK_HOME_DIR + "/.write_test";
        FILE *test_fp = fopen(test_file.c_str(), "w");
        if (test_fp != nullptr) {
            fclose(test_fp);
            fs::remove(test_file);
            return CPK_HOME_DIR;
        }
    } else {
        // Try to create CPK_HOME_DIR if it doesn't exist
        try {
            fs::create_directories(CPK_HOME_DIR);
            // Test if we can write
            std::string test_file = CPK_HOME_DIR + "/.write_test";
            FILE *test_fp = fopen(test_file.c_str(), "w");
            if (test_fp != nullptr) {
                fclose(test_fp);
                fs::remove(test_file);
                return CPK_HOME_DIR;
            }
        } catch (const std::exception& e) {
            // Can't create or write to CPK_HOME_DIR, fall through to user cache
        }
    }

    // If we couldn't use CPK_HOME_DIR, fallback to user cache directory (~/.cpk)
    const char* home = std::getenv("HOME");
    if (home != nullptr) {
        std::string user_cache = std::string(home) + "/.cpk";
        // Ensure the directory exists
        try {
            fs::create_directories(user_cache);
            if (CPK_VERBOSE) {
                print_message("Using user cache directory: " + user_cache, YELLOW);
            }
            return user_cache;
        } catch (const std::exception& e) {
            // If we can't create user cache dir, use CPK_HOME_DIR anyway
            return CPK_HOME_DIR;
        }
    }

    // If all else fails, return CPK_HOME_DIR
    return CPK_HOME_DIR;
}

// Function to get cache file path, trying CPK_HOME_DIR first, then falling back to ~/.cpk
std::string get_cache_file(const std::string &filename) {
    std::string cache_dir = get_cache_dir();
    return cache_dir + "/" + filename;
}