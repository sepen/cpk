#include "../cpk.h"
#include "../utils.h"
#include "cmd_info.h"
#include <vector>
#include <string>

void cmd_deps(const std::vector<std::string>& args) {
    if (args.empty()) {
        print_message("Package name is required", RED);
        return;
    }
    
    // Call cmd_info with --dependencies flag
    std::vector<std::string> info_args = {args[0], "--dependencies"};
    cmd_info(info_args);
}
