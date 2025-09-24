#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <vector>

const std::string RED     = "\033[31m"; // error
const std::string GREEN   = "\033[32m"; // success
const std::string BLUE    = "\033[34m"; // header
const std::string YELLOW  = "\033[33m"; // not success
const std::string BOLD    = "\033[1m";
const std::string NONE    = "\033[0m";
const std::string NEWLINE = "\n";

std::string url_encode(const std::string& value);
std::string url_decode(const std::string& value);
std::string ltrim(const std::string& str);
static size_t write_data(void *ptr, size_t size, size_t nmemb, FILE *stream);
bool download_file(const std::string& url, const std::string &file_path, bool overwrite = false);
bool prompt_user(const std::string &file_path);
bool extract_package(const std::string &tar_file, const std::string &dest_dir);
bool parse_pkgfile(const std::string &pkgfile_path, std::string &pkgname, std::string &pkgdesc, std::string &pkgurl, std::string &pkgdeps);
int shellcmd(const std::string& command, const std::vector<std::string>& args, std::string* output, bool show_output = true);
bool run_script(const std::string& script_path, const std::string& msg);
void print_header(const std::string &message, const std::string &color = NONE);
void print_message(const std::string &message, const std::string &color = NONE);
void print_version();
void print_help();
bool output_colors_enabled();
bool load_cpk_config(const std::string &config_file);
bool directory_exists(const std::string &path);
std::string find_pkg_file(const std::string& directory, const std::string& pkgname, const std::string& pkgver);
bool find_package(const std::string& package_name, std::string& package, std::string& pkgname, std::string& pkgver, std::string& pkgarch);
bool is_package_installed(const std::string& package_name);
int get_number_of_packages();
bool change_directory(const std::string& path);
void print_fmt_header(const std::string& header_text);
void print_fmt_lines(const std::string& text);
std::vector<std::string> find_public_keys(const std::string& directory);

#endif  // UTILS_H

