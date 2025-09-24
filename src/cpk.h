#ifndef CPK_H
#define CPK_H

#include <string>
#include <vector>

extern const std::string CPK_VERSION;

// Global variables (set via config file)
extern std::string CPK_CONF_FILE;
extern std::string CPK_REPO_URL;
extern std::string CPK_HOME_DIR;
extern std::string CPK_INSTALL_ROOT;
extern std::string CPK_PKGMK_CMD;
extern std::string CPK_PKGADD_CMD;
extern std::string CPK_PKGRM_CMD;
extern std::string CPK_PKGINFO_CMD;

extern bool CPK_COLOR_MODE;
extern bool CPK_VERBOSE;

// Function prototypes for package management commands
void cmd_update(const std::vector<std::string>& args);
void cmd_search(const std::vector<std::string>& args);
void cmd_info(const std::vector<std::string>& args);
void cmd_install(const std::vector<std::string>& args);
void cmd_uninstall(const std::vector<std::string>& args);
void cmd_upgrade(const std::vector<std::string>& args);
void cmd_list(const std::vector<std::string>& args);
void cmd_diff(const std::vector<std::string>& args);
void cmd_build(const std::vector<std::string>& args);
void cmd_verify(const std::vector<std::string>& args);
void cmd_clean(const std::vector<std::string>& args);

#endif // CPK_H
