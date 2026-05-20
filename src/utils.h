#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <vector>
#include <filesystem>
#include <algorithm>

namespace fs = std::filesystem;

extern const std::string RED;
extern const std::string GREEN;
extern const std::string BLUE;
extern const std::string YELLOW;
extern const std::string BOLD;
extern const std::string NONE;
extern const std::string NEWLINE;

// 
std::string url_encode(const std::string& value);
std::string url_decode(const std::string& value);
// Append path under CPK_REPO_URL (avoids "//" when the configured URL ends with "/")
std::string cpk_repo_join(const std::string& path_component);
std::string ltrim(const std::string& str);

// Compare versions semantically
int compare_versions(const std::string& v1, const std::string& v2);

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
void print_help(const std::string& command = "");
void print_general_options();
bool output_colors_enabled();
bool load_cpk_config(const std::string &config_file);
bool directory_exists(const std::string &path);
std::string find_pkg_file(const std::string& directory, const std::string& pkgname, const std::string& pkgver);
bool find_package(const std::string& package_name, std::string& package, std::string& pkgname, std::string& pkgver, std::string& pkgarch);
bool parse_cpk_filename(const std::string& filepath, std::string& pkgname, std::string& pkgver, std::string& pkgarch);
bool get_package_dependency_names(const std::string& spec, std::vector<std::string>& out);
bool is_package_installed(const std::string& package_name);
int get_number_of_packages();
bool change_directory(const std::string& path);
void print_fmt_header(const std::string& header_text);
void print_fmt_lines(const std::string& text);
std::vector<std::string> find_public_keys(const std::string& directory);
void ensure_directory(const fs::path &dir);
std::vector<std::string> get_local_files(const std::vector<std::string> &sources);
void copy_files(const fs::path &source_dir, const fs::path &dest_dir, const std::vector<std::string> &files);
void package_files(const std::string &name, const std::string &version, const std::string &release, const std::string &arch, const fs::path &output_dir);
void generate_cpk_index(const fs::path &repo_dir);
std::string get_system_architecture();
std::vector<std::string> get_installed_packages();
std::string calculate_sha256(const std::string &file_path);
bool parse_cpk_info(const std::string &info_file_path, std::string &name, std::string &version, std::string &arch, std::string &description, std::string &url, std::string &dependencies);
// Writable cache (~/.cpk when CPK_HOME_DIR is not writable): .info, .cpk downloads, extracted trees.
std::string get_cache_dir();
std::string get_cache_file(const std::string &filename);
// System package index (cpk_home_dir/CPKINDEX); read-only commands use this path only.
std::string get_cpkindex_path();
void cpk_print_missing_index_error();
// True if running as root (privileged commands).
bool cpk_is_privileged_process();
bool cpk_file_readable(const std::string& path);
// Prefer cpk_home_dir/<basename> if that .info (etc.) is readable; else user cache path.
std::string resolve_cpk_metadata_read_path(const std::string& basename);
// Prefer extracted tree under cpk_home_dir when it has a Pkgfile; else under get_cache_dir().
std::string resolve_package_extract_dir(const std::string& pkgname, const std::string& pkgver);

#endif  // UTILS_H

