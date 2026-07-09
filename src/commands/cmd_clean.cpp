#include "../cpk.h"
#include "../utils.h"
#include "../fs_compat.h"

void cmd_clean(const std::vector<std::string>& args) {

    // As root: clean system cpk home except CPKINDEX; otherwise clean user cache only.
    std::string cache_dir = cpk_is_privileged_process() ? CPK_HOME_DIR : get_cache_dir();

    if (CPK_VERBOSE) {
        print_header("Cleaning cache contents", BLUE);
    }

    if (fs::exists(cache_dir) && fs::is_directory(cache_dir)) {
        // Iterate over directory contents and remove them
        for (const auto& entry : fs::directory_iterator(cache_dir)) {
            if (entry.path().filename() == "CPKINDEX") {
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
