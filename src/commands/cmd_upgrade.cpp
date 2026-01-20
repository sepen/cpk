#include "../cpk.h"
#include "../utils.h"
#include "cmd_install.h"
#include <sstream>
#include <vector>
#include <string>

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
                cmd_install({pkgname, "--upgrade"});
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
