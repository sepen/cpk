#include "../cpk.h"
#include "../utils.h"
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

void cmd_update(const std::vector<std::string>& args) {
    if(!fs::is_directory(CPK_HOME_DIR)) {
        print_message("Home directory does not exists " + CPK_HOME_DIR, RED);
        return;
    }

    const std::string index_file = CPK_HOME_DIR + "/CPKINDEX";
    if (CPK_VERBOSE) {
        if (!fs::exists(index_file)) {
            print_header("Initializing index of available packages", BLUE);
        }
        else {
            print_header("Updating index of available packages", BLUE);
        }
    }

    const std::string index_url = CPK_REPO_URL + "/CPKINDEX";
    if (!download_file(index_url, index_file, true)) {
        print_message("Failed to update index file " + index_file, RED);
        return;
    }

    int package_count = get_number_of_packages();
    print_message(std::to_string(package_count)+ " packages available");
}
