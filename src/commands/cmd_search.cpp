#include "../cpk.h"
#include "../utils.h"
#include <filesystem>
#include <fstream>
#include <sstream>

namespace fs = std::filesystem;

void cmd_search(const std::vector<std::string>& args) {
    if (args.empty()) {
        print_message("Search argument is required");
        return;
    }
    std::string search_term = args[0];

    std::string index_file = CPK_HOME_DIR + "/CPKINDEX";
    if (!fs::exists(index_file)) {
        print_message("Package index not found. Run `cpk update` first", RED);
        return;
    }

    std::ifstream file(index_file);
    std::string index_line;
    bool found = false;

    std::ostringstream search_results;
    while (std::getline(file, index_line)) {
        if (index_line.find(search_term) != std::string::npos) {
            found = true;
            search_results << index_line << '\n';
        }
    }
    file.close();

    if (!found) {
        print_message("No matching packages found", YELLOW);
    } else {
        print_fmt_lines(search_results.str());
    }

    return;
}
