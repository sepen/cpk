#include "../cpk.h"
#include "../utils.h"
#include <filesystem>
#include <vector>
#include <string>

namespace fs = std::filesystem;

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
