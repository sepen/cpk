#include "../cpk.h"
#include "../utils.h"
#include <vector>
#include <string>

void cmd_list(const std::vector<std::string>& args) {
    std::vector<std::string> pkginfo_args = { "-i" };
    std::string pkginfo_output;

    if (shellcmd(CPK_PKGINFO_CMD, pkginfo_args, &pkginfo_output, false) != 0) {
        print_message("Failed to get list of installed packages", RED);
    }
    else {
        if (CPK_VERBOSE) {
            print_header("Printing list of installed packages", BLUE);
        }
        print_fmt_header("Package Version");
        print_fmt_lines(pkginfo_output);
    }

    return;
}
