#include "../cpk.h"
#include "../utils.h"
#include <filesystem>

namespace fs = std::filesystem;

void cmd_index(const std::vector<std::string>& args) {
    if (args.size() != 1) {
        return;
    }
    fs::path repo_dir = args[0];
    if (!fs::is_directory(repo_dir)) {
        print_message("Directory does not exist: " + repo_dir.string(), RED);
        return;
    }
    if (CPK_VERBOSE) {
        print_header("Updating index of local repository", BLUE);
    }
    generate_cpk_index(repo_dir);
    return;
}
