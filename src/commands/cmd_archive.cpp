#include "../cpk.h"
#include "../utils.h"
#include <filesystem>
#include <fstream>
#include <vector>
#include <string>

namespace fs = std::filesystem;

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
