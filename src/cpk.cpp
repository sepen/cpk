#include "cpk.h"
#include "utils.h"
#include "commands/cmd_update.h"
#include "commands/cmd_info.h"
#include "commands/cmd_deps.h"
#include "commands/cmd_deptree.h"
#include "commands/cmd_search.h"
#include "commands/cmd_list.h"
#include "commands/cmd_diff.h"
#include "commands/cmd_verify.h"
#include "commands/cmd_build.h"
#include "commands/cmd_install.h"
#include "commands/cmd_uninstall.h"
#include "commands/cmd_upgrade.h"
#include "commands/cmd_clean.h"
#include "commands/cmd_index.h"
#include "commands/cmd_archive.h"
#include <fstream>
#include <iostream>
#include <filesystem>
#include <set>
#include <sstream>
#include <system_error>

namespace fs = std::filesystem;

const std::string CPK_VERSION = "0.3";

std::string CPK_CONF_FILE = "/etc/cpk.conf";
std::string CPK_REPO_URL = "https://cpk.user.ninja";
std::string CPK_HOME_DIR = "/var/lib/cpk";
std::string CPK_INSTALL_ROOT = "/";
std::string CPK_PKGMK_CMD = "pkgmk";
std::string CPK_PKGADD_CMD = "pkgadd";
std::string CPK_PKGRM_CMD = "pkgrm";
std::string CPK_PKGINFO_CMD = "pkginfo";

bool CPK_COLOR_MODE = false;
bool CPK_VERBOSE = false;

int main(int argc, char* argv[]) {
    // Parse command-line arguments
    std::vector<std::string> command_args;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if ((arg == "--config" || arg == "-c") && i + 1 < argc) {
            CPK_CONF_FILE = argv[++i];  // Increment i after consuming argument
        } else if ((arg == "--root" || arg == "-r") && i + 1 < argc) {
            CPK_INSTALL_ROOT = argv[++i];
        } else if ((arg == "--color" || arg == "-C")) {
            CPK_COLOR_MODE = true;
        } else if ((arg == "--verbose" || arg == "-v")) {
            CPK_VERBOSE = true;
        } else if (arg == "--help" || arg == "-h") {
            print_help("");
            return 0;
        } else {
            command_args.push_back(arg);  // Collect remaining command arguments
        }
    }

    // If no command is provided, print help
    if (command_args.empty()) {
        print_help("");
        return 1;
    }

    // Load the configuration file
    if (!load_cpk_config(CPK_CONF_FILE)) {
        if (CPK_VERBOSE) {
            print_message("Failed to load config file " + CPK_CONF_FILE, RED);
        }
        return 1;
    }

    // Get the first argument as the command and the rest as its arguments
    std::string command = command_args[0];
    std::vector<std::string> args(command_args.begin() + 1, command_args.end());

    // Execute the corresponding command
    if (command == "update") {
        cmd_update(args);
    } else if (command == "info") {
        cmd_info(args);
    } else if (command == "deps") {
        cmd_deps(args);
    } else if (command == "deptree") {
        cmd_deptree(args);
    } else if (command == "search") {
        cmd_search(args);
    } else if (command == "list") {
        cmd_list(args);
    } else if (command == "diff") {
        cmd_diff(args);
    } else if (command == "verify") {
        cmd_verify(args);
    } else if (command == "build") {
        cmd_build(args);
    } else if (command == "install" || command == "add") {
        cmd_install(args);
    } else if (command == "uninstall" || command == "del" || command == "rm") {
        cmd_uninstall(args);
    } else if (command == "upgrade") {
        cmd_upgrade(args);
    } else if (command == "clean") {
        cmd_clean(args);
    } else if (command == "index") {
        cmd_index(args);
    } else if (command == "archive") {
        cmd_archive(args);
    } else if (command == "help") {
        if (args.empty()) {
            print_help("");
        } else {
            print_help(args[0]);
        }
    } else if (command == "version") {
        print_version();
    } else {
        print_help("");
    }

    return 0;
}
