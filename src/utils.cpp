#include "cpk.h"
#include "utils.h"
#include <archive.h>
#include <archive_entry.h>
#include <curl/curl.h>
#include <sys/stat.h>
#include <fstream>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <cstdlib>
#include <stdexcept>
#include <algorithm>
#include <filesystem>
#include <string>

namespace fs = std::filesystem;

// Helper function to remove left leading spaces from a string
std::string ltrim(const std::string& str) {
    std::string s = str;
    s.erase(0, s.find_first_not_of(' '));  // Removes leading spaces
    return s;
}

// Helper function to write the downloaded content to a file
static size_t write_data(void *ptr, size_t size, size_t nmemb, FILE *stream) {
    size_t written = fwrite(ptr, size, nmemb, stream);
    return written;
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

    print_message("Fetching " + url_decode(url));
    
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
    if (res != CURLE_OK) {
        print_message("Download error: " + std::string(curl_easy_strerror(res)), RED);
    }

    // Clean up
    curl_easy_cleanup(curl);
    fclose(fp);

    return res == CURLE_OK;
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

// Display the help message
void print_help() {
    print_message("\033[1mCRUX Package Keeper - package management tool for CRUX Linux\033[0m");

    print_message("\nUsage:");
    print_message("  cpk <command> [options]");

    print_message("\nCommands:");
    print_message("  update                 Update the index of available packages");
    print_message("  info <package>         Show information about installed or available packages");
    print_message("  search <word|package>  Search for packages by name or keyword");
    print_message("  install <package>      Install or upgrade packages on the system");
    print_message("  uninstall <package>    Remove packages from the system");
    print_message("  upgrade                Upgrade all installed packages to the latest versions");
    print_message("  list                   List all installed packages");
    print_message("  verify <package>       Verify integrity of package source files");
    print_message("  help                   Show this help message");

    print_message("\nOptions:");
    print_message("  -r, --root <path>      Specify an alternative installation root directory");
    print_message("  -v, --verbose <int>    Set the verbosity level for output messages (default: 0)");
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
bool parse_pkgfile(const std::string& pkgfile_path, std::string& pkgname, std::string& pkgdesc, std::string& pkgurl, std::string& pkgdeps) {
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
            pkgname = line.substr(5); // Extract after 'name='
        } else if (line.rfind("# Description:", 0) == 0) {
            pkgdesc = line.substr(14); // Extract after '# Description:'
        } else if (line.rfind("# URL:", 0) == 0) {
            pkgurl = line.substr(6); // Extract after '# URL:'
        } else if (line.rfind("# Depends on:", 0) == 0) {
            pkgdeps = line.substr(13); // Extract after '# Depends on:'
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
         
        if (key == "cpk_home_dir") {
            iss >> CPK_HOME_DIR;
        } else if (key == "cpk_repo_url") {
            iss >> CPK_REPO_URL;
        } else if (key == "cpk_color_mode") {
            std::string colors;
            iss >> colors;
            CPK_COLOR_MODE = (colors == "true");
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
    if (CPK_INSTALL_ROOT == "/") {
        print_message("Skipping script " + script_path, YELLOW);
    } else {
        print_header(msg, GREEN);
        if (shellcmd("sh -x", {script_path}, nullptr) != 0) {
            print_message("Failed to execute script " + script_path, RED);
            return false;
        }
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
    std::string index_file = CPK_HOME_DIR + "/CPKINDEX";
    bool result = false;

    if (!fs::exists(index_file)) {
        print_message("Package index not found. Run `cpk update` first", RED);
        result = false;
    } else {
        std::ifstream file(index_file);  // Open the CPKINDEX file for reading
        if (!file.is_open()) {
            print_message("Error opening file: " + index_file, RED);
            result = false;  // Return an error code if the file cannot be opened
        }
        else {
            std::string index_line;
            while (std::getline(file, index_line)) {
                if (index_line.find(package_name + "#") != std::string::npos) {
                    size_t cpk_pos = index_line.rfind(".cpk");
                    size_t last_dot = index_line.rfind('.', cpk_pos - 1);
                    size_t hash_pos = index_line.find('#');

                    if (cpk_pos != std::string::npos && last_dot != std::string::npos && hash_pos != std::string::npos) {
                        pkgname = index_line.substr(0, hash_pos);
                        pkgver = index_line.substr(hash_pos + 1, last_dot - hash_pos - 1);
                        pkgarch = index_line.substr(last_dot + 1, cpk_pos - last_dot - 1);
                        package = index_line;
                        if (package_name == pkgname) {
                            result = true;
                        }
                    }
                }
            }
        }
        file.close();  // Close the file
    }

    if (!result) {
        print_message("Package not found: " + package_name, RED);
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
    std::string index_file = CPK_HOME_DIR + "/CPKINDEX";
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