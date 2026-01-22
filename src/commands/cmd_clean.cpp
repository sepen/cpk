#include "../cpk.h"
#include "../utils.h"
#include <filesystem>

namespace fs = std::filesystem;

void cmd_clean(const std::vector<std::string>& args) {

    std::string excluded_file = "CPKINDEX";
    std::string cache_dir = get_cache_dir();

    if (CPK_VERBOSE) {
        print_header("Cleaning cache contents", BLUE);
    }

    if (fs::exists(cache_dir) && fs::is_directory(cache_dir)) {
        // Iterate over directory contents and remove them
        for (const auto& entry : fs::directory_iterator(cache_dir)) {
            if (entry.path().filename() == excluded_file) {
                continue;
            }
            fs::remove_all(entry);
        }
        print_message("Cache contents deleted successfully");
    } else {
        print_message("Path does not exists or is not a directory " + cache_dir, RED);
    }

    return;
}
