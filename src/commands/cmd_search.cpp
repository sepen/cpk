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

    const std::string index_file = get_cpkindex_path();
    std::ifstream file(index_file);
    if (!file.is_open()) {
        cpk_print_missing_index_error();
        return;
    }
    std::string index_line;
    bool found = false;

    std::ostringstream search_results;
    while (std::getline(file, index_line)) {
        if (!cpk_index_line_valid(index_line)) {
            continue;
        }
        const std::string pkg = cpk_index_line_package(index_line);
        if (pkg.find(search_term) != std::string::npos) {
            found = true;
            search_results << pkg << '\n';
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
