#include "../cpk.h"
#include "../utils.h"
#include <filesystem>
#include <vector>
#include <string>

namespace fs = std::filesystem;

void cmd_uninstall(const std::vector<std::string>& args) {

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

    if (!is_package_installed(pkgname)) {
        print_message("Package " + pkgname + " not installed", RED);
        return;
    }

    print_header("Uninstalling package " + pkgname, BLUE);

    std::vector<std::string> pkgrm_args = { "-r", CPK_INSTALL_ROOT, pkgname };
    std::string pkgrm_output;

    if (CPK_VERBOSE) {
        print_message("Running " + CPK_PKGRM_CMD + " -r " + CPK_INSTALL_ROOT + " " + pkgname);
    }

    if (shellcmd(CPK_PKGRM_CMD, pkgrm_args, &pkgrm_output) != 0) {
        print_message("Failed to uninstall package", RED);
        return;
    }
    return;
}
