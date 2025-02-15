#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <vector>

// Define color constants
const std::string RED    = "\033[31m";
const std::string GREEN  = "\033[32m";
const std::string BLUE   = "\033[34m";
const std::string YELLOW = "\033[33m";
const std::string NONE   = "\033[0m";  // Reset color to default

std::string url_encode(const std::string& value);

std::string ltrim(const std::string& str);

// Function to download a file from a URL
bool download_file(const std::string& url, const std::string &file_path);

// Prompt user for confirmation
bool prompt_user(const std::string &file_path);

// Extracts a tarball to the specified destination directory
bool extract_package(const std::string &tar_file, const std::string &dest_dir);

// Parses the Pkgfile to extract metadata
bool parse_pkgfile(const std::string &pkgfile_path, std::string &pkgname, std::string &pkgdesc, std::string &pkgurl, std::string &pkgdeps);

// Function to execute a shell command and return the output
int shellcmd(const std::string& command, const std::vector<std::string>& args, std::string* output, bool show_output = true);

// Helper function to run scripts
bool run_script(const std::string& script_path, const std::string& msg);

// Function to print colored messages to the console (if enabled in config)
void print_message(const std::string &message, const std::string &color);

// Function to print help information and usage
void print_help();

// Function to check if output colors should be enabled (based on config)
bool output_colors_enabled();

// Function to load configuration from a config file (cpk.conf)
bool load_cpk_config(const std::string &config_file);

// Function to check if a directory exists
bool directory_exists(const std::string &path);

// Function to fetch the cpk repository URL
std::string get_cpk_repo_url();

// Function to fetch the cpk directory path
std::string get_cpk_dir();

// Function to find compression mode and the full path for package file
std::string find_pkg_file(const std::string& directory, const std::string& pkgname, const std::string& pkgver);

// Helper function to find package details
bool find_package(const std::string& package_name, std::string& package, std::string& pkgname, std::string& pkgver, std::string& pkgarch);

int get_number_of_packages();

bool is_package_installed(const std::string& package_name);

#endif  // UTILS_H

