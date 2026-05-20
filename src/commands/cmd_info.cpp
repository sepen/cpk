#include "../cpk.h"
#include "../utils.h"
#include <filesystem>
#include <fstream>
#include <sstream>

namespace fs = std::filesystem;

void cmd_info(const std::vector<std::string>& args) {
    if (args.empty()) {
        print_message("Package name is required", RED);
        return;
    }

    std::string pkg_name = args[0];
    std::string field_filter;  // Empty = show all, or specific field like "name", "url", etc.
    
    // Parse optional field filter
    if (args.size() > 1) {
        field_filter = args[1];
        // Remove leading dashes
        if (field_filter[0] == '-') {
            field_filter = field_filter.substr(1);
        }
        if (field_filter[0] == '-') {
            field_filter = field_filter.substr(1);
        }
    }

    std::string package, pkgname, pkgver, pkgarch;
    if (!find_package(pkg_name, package, pkgname, pkgver, pkgarch)) {
        return;
    }

    // Prefer .cpk.info under cpk_home_dir; otherwise download into user/writable cache
    std::string info_url = cpk_repo_join(url_encode(package + ".info"));
    const std::string info_bn = package + ".info";
    const std::string info_read = resolve_cpk_metadata_read_path(info_bn);
    const std::string info_write = get_cache_file(info_bn);
    bool info_from_file = false;

    std::string name, version, arch, description, url, dependencies;

    if (cpk_file_readable(info_read) && parse_cpk_info(info_read, name, version, arch, description, url, dependencies)) {
        info_from_file = true;
    } else if (download_file(info_url, info_write, true) && parse_cpk_info(info_write, name, version, arch, description, url, dependencies)) {
        info_from_file = true;
    } else if (fs::exists(info_write)) {
        fs::remove(info_write);
    }

    // Fallback: download .cpk and extract info from Pkgfile
    if (!info_from_file) {
        std::string cache_dir = get_cache_dir();
        std::string package_url = cpk_repo_join(url_encode(package));
        std::string package_source = resolve_package_extract_dir(pkgname, pkgver);
        std::string package_path = get_cache_file(package);

        if (!fs::is_directory(package_source) || !fs::exists(package_source + "/Pkgfile")) {
            package_source = cache_dir + "/" + pkgname + "/" + pkgver;
            if (!download_file(package_url, package_path) || !extract_package(package_path, cache_dir)) {
                print_message("Failed to retrieve package info", RED);
                return;
            }
        }
        
        // Parse Pkgfile to get package information
        std::string pkgfile_path = package_source + "/Pkgfile";
        if (!fs::exists(pkgfile_path)) {
            print_message("Package info file not found and Pkgfile not available", RED);
            return;
        }
        
        std::string pkgname_from_file, pkgdesc, pkgurl, pkgdeps;
        if (!parse_pkgfile(pkgfile_path, pkgname_from_file, pkgdesc, pkgurl, pkgdeps)) {
            print_message("Failed to parse Pkgfile", RED);
            return;
        }
        
        // Read version and release from Pkgfile
        std::ifstream pkgfile(pkgfile_path);
        std::string pkgversion, pkgrelease;
        std::string line;
        while (std::getline(pkgfile, line)) {
            std::string trimmed_line = line;
            trimmed_line.erase(0, trimmed_line.find_first_not_of(" \t"));
            trimmed_line.erase(trimmed_line.find_last_not_of(" \t") + 1);
            
            if (trimmed_line.find("version=") == 0) {
                pkgversion = trimmed_line.substr(8);
            } else if (trimmed_line.find("release=") == 0) {
                pkgrelease = trimmed_line.substr(8);
            }
        }
        pkgfile.close();
        
        // Set values from Pkgfile
        name = pkgname_from_file;
        version = pkgversion + "-" + pkgrelease;
        arch = pkgarch;
        description = pkgdesc;
        url = pkgurl;
        dependencies = pkgdeps;
    }

    // Show information based on filter
    if (field_filter.empty()) {
        // Show all fields
        print_message(BOLD + "Name         " + NONE + "| " + name);
        print_message(BOLD + "Version      " + NONE + "| " + version);
        print_message(BOLD + "Arch         " + NONE + "| " + arch);
        print_message(BOLD + "Description  " + NONE + "| " + description);
        print_message(BOLD + "URL          " + NONE + "| " + url);
        print_message(BOLD + "Dependencies " + NONE + "| " + dependencies);
    }
    else if (field_filter == "name") {
        print_message(name);
    }
    else if (field_filter == "version") {
        print_message(version);
    }
    else if (field_filter == "arch") {
        print_message(arch);
    }
    else if (field_filter == "description") {
        print_message(description);
    }
    else if (field_filter == "url") {
        print_message(url);
    }
    else if (field_filter == "dependencies") {
        if (dependencies.empty()) {
            print_message("No dependencies", GREEN);
        } else {
            print_message(dependencies);
        }
    }
    else {
        print_message("Unknown field: " + field_filter, RED);
    }
}
