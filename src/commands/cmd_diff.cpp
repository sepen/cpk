#include "../cpk.h"
#include "../utils.h"
#include <sstream>
#include <vector>
#include <string>

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
