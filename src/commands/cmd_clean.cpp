#include "../cpk.h"
#include "../utils.h"
#include <filesystem>

namespace fs = std::filesystem;

void cmd_clean(const std::vector<std::string>& args) {

    std::string excluded_file = "CPKINDEX";

    if (CPK_VERBOSE) {
        print_header("Cleaning cache contents", BLUE);
    }

    if (fs::exists(CPK_HOME_DIR) && fs::is_directory(CPK_HOME_DIR)) {
        // Iterate over directory contents and remove them
        for (const auto& entry : fs::directory_iterator(CPK_HOME_DIR)) {
            if (entry.path().filename() == excluded_file) {
                continue;
            }
            fs::remove_all(entry);
        }
        print_message("Cache contents deleted successfully");
    } else {
        print_message("Path does not exists or is not a directory " + CPK_HOME_DIR, RED);
    }

    return;
}
