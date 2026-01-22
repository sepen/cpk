# CRUX Package Keeper (cpk)

CRUX Package Keeper is a toolset designed to efficiently manage packages and their sources in CRUX environments.


## Features

- Centralized and standardized package + source management.
- Safe reuse of pre-built packages across different machines.
- Clear metadata about the source and build process of each package.
- Native multi-architecture support with explicit architecture identifiers.


## Motivation

In CRUX, binary packages (`.pkg.tar.gz`, `.pkg.tar.bz2`, `.pkg.tar.xz`) are intentionally minimal. While this design keeps packages lightweight, it also comes with some drawbacks:

- No record of the build-time dependencies used during compilation.
- No inclusion of the local sources provided by the port, which are sometimes required to build or properly configure the package.
- No pre-install or post-install scripts that may be required for proper setup.
- No inclusion of the README file that often provides important usage or configuration notes.
- No explicit reference to the target architecture, which makes it harder to manage multi-architecture environments (e.g., `x86_64`, `arm64`, `armhf`).

Because of these limitations, binary packages alone are not fully self-descriptive. Installing them without access to the corresponding port source may reduce portability and reliability, as important contextual information is missing.

`cpk` addresses these issues by embedding the complete port source within the package and extending the package format to include the target architecture in its name. This makes packages:
- **Self-contained** — all relevant metadata and sources are included.
- **Reproducible** — rebuilds can be performed reliably using the embedded port.
- **Auditable** — users can inspect the exact sources and instructions used to build the package.
- **Multi-architecture aware** — different builds of the same package (e.g., `pkgname#1.0-1.x86_64.cpk`, `pkgname#1.0-1.arm64.cpk`).


## Relation to pkgutils

`cpk` is not a replacement for `pkgutils` but an extension built on top of it.
All low-level package management tasks (installation, removal, querying, etc.) are still handled by `pkgadd`, `pkgrm`, `pkginfo`, `pkgmk` and related tools from `pkgutils`.

What `cpk` adds on top is:

- A richer package format (`.cpk`) that embeds port sources, metadata, and architecture information.
- Higher-level commands (`cpk search`, `cpk verify`, `cpk diff`, etc.) that extend the functionality of `pkgutils`.
- Improved portability and reproducibility while maintaining full compatibility with the existing CRUX ecosystem.

In short, `pkgutils` does the heavy lifting, while `cpk` provides the missing context and metadata.


## Basic Usage

```
CRUX Package Keeper - package management tool for CRUX Linux

Usage:
  cpk <command> [options]
  cpk help <command>   Show detailed help for a specific command

Commands:
  update      Update the index of available packages
  info        Show information about installed or available packages
  deps        Show package dependencies
  search      Search for packages by name or keyword
  list        List all installed packages
  diff        Show differences between installed and available packages
  verify      Verify integrity of package source files
  build       Build a package from source files
  install     Install or upgrade packages on the system
  add         Alias for install
  uninstall   Remove packages from the system
  del         Alias for uninstall
  rm          Alias for uninstall
  upgrade     Upgrade all installed packages to the latest versions
  clean       Clean up package source files and temporary directories
  index       Create a CPKINDEX file for a local repository
  archive     Create .cpk archive(s) from a directory containing ports
  help        Show this help message or detailed help for a command
  version     Show version information

General Options:
  -c, --config <file>      Set an alternative configuration file (default: /etc/cpk.conf)
  -r, --root <path>        Set an alternative installation root (default: /)
  -C, --color              Show colorized output messages
  -v, --verbose            Show verbose output messages
  -h, --help               Print this help information
```

For detailed help on a specific command, use `cpk help <command>`. For example:
- `cpk help info` - Shows detailed information about the `info` command and its field options
- `cpk help install` - Shows detailed information about the `install` command and `--upgrade` option

## Command Reference

### `cpk update`

**Usage**: (no arguments)

- Ensures `CPK_HOME_DIR` exists; otherwise prints an error.
- Downloads `CPKINDEX` from `${CPK_REPO_URL}/CPKINDEX` into `CPK_HOME_DIR`.
- Counts available packages and prints the total.

### `cpk info <package> [--field]`

**Usage**: one required argument (package name), optional field filter

- Finds the package in `CPKINDEX`.
- Attempts to download the `.cpk.info` file directly from the repository (faster).
- If `.cpk.info` is not available, falls back to downloading and extracting the `.cpk` archive and parsing the `Pkgfile`.
- Displays all fields by default: Name, Version, Arch, Description, URL, and Dependencies.
- Field filters: `--name`, `--version`, `--arch`, `--description`, `--url`, `--dependencies`
- Examples:
  - `cpk info vim` - Shows all package information
  - `cpk info vim --version` - Shows only the version
  - `cpk info vim --dependencies` - Shows only dependencies

### `cpk deps <package>`

**Usage**: one argument (package name)

- Shows package dependencies (alias for `cpk info <package> --dependencies`).
- Finds the package in `CPKINDEX` and displays its dependencies.

### `cpk search <term>`

**Usage**: one argument (substring to match)

- Opens `CPKINDEX` and searches all lines containing the given term.
- Displays all matches in a formatted list, or shows a "no matches found" message.

### `cpk list`

**Usage**: (no arguments)

- Runs `pkginfo -i` to list installed packages on the system.
- Prints a formatted table of package names and versions.

### `cpk diff`

**Usage**: (no arguments)

- Loads installed packages using `pkginfo -i`.
- For each one, looks up the version in the remote index (`CPKINDEX`).
- Shows all entries where the local version differs from the repository version.

### `cpk verify <package>`

**Usage**: one argument (package name or key)

- Locates the package in the index and downloads it if missing.
- Runs `pkgmk -do` to ensure all source files are fetched.
- Attempts to verify the `.signature` using public keys found under `/etc/ports/`.
- Reports success if any key verifies, otherwise shows an error.

### `cpk build <package>`

**Usage**: one argument (package name or key)

- Finds the package and downloads/extracts it if needed.
- Runs `pkgmk -d` inside the source directory to build the package.
- Prints a success message or an error if the build fails.

### `cpk install <package> [--upgrade]`
### `cpk install <path/to/package.cpk> [--upgrade]`
### `cpk add <package> [--upgrade]`
### `cpk add <path/to/package.cpk> [--upgrade]`

**Usage**: one required argument (package name or path to .cpk file), optional `--upgrade` flag

- Can install from repository or from a local .cpk file.
- If installing from repository:
  - Finds the package in the repository index.
- If installing from local file:
  - Parses the .cpk filename to extract package name, version, and architecture.
  - Extracts the package to access pre/post install scripts and README.
- If already installed:
  - Continues only if `--upgrade` is specified; otherwise prints a warning.
- Ensures the package archive exists locally.
- Executes pre-install and post-install scripts when present.
- Runs `pkgadd -r <CPK_INSTALL_ROOT> [ -u ] <package_file>` to install or upgrade.
- Displays the README (if available) after installation.
- `add` is an alias for `install`.
- Examples:
  - `cpk install vim` - Install from repository
  - `cpk install /tmp/mypackage#4.1.0-1.i686.cpk` - Install from local file
  - `sudo cpk add /tmp/mypackage#4.1.0-1.i686.cpk` - Install from local file with sudo

### `cpk uninstall <package>`
### `cpk del <package>`
### `cpk rm <package>`

**Usage**: one argument (package name)

- Checks if the package is installed; exits if not.
- Executes `pkgrm -r <CPK_INSTALL_ROOT> <pkgname>` to remove it.
- Reports errors or success accordingly.
- `del` and `rm` are aliases for `uninstall`.

### `cpk upgrade [<package>...]`

**Usage**: zero or more package names

- If no arguments: upgrades all installed packages that have newer versions available.
- If package names are provided: upgrades only those specific packages.
- For each package:
  - Checks if an update exists in the repo and calls `cpk install --upgrade` when needed.
  - Skips missing or uninstalled packages and logs them in verbose mode.

### `cpk clean`

**Usage**: (no arguments)

- Scans `CPK_HOME_DIR` and removes all files and directories except `CPKINDEX`.
- Prints status messages for each deletion and confirms cleanup completion.

### `cpk index <repo>`

**Usage**: one argument (local repository path)

- Validates that the argument is a directory.
- Rebuild or update the local `CPKINDEX`.

### `cpk archive <portsdir> <repo>`

**Usage**: two arguments (<portsdir> <repo>)

- Recursively scans `<portsdir>` for built packages (*.pkg.tar.gz, .bz2, .xz).
- Reads Pkgfile in each directory to extract name, version, release, and source fields.
- Validates package filename format (`name#version-release.pkg.tar.*`).
- Copies files and metadata into `<repo>/<name>/<version-release>` and creates `.cpk` archives.
- Prints progress and summary messages (verbose mode supported).


## Contributing

If you want to contribute to this project, feel free to fork the repository, make your changes, and submit a pull request. Contributions from the community are always welcome!
See the [CONTRIBUTING](CONTRIBUTING) file for details.

## License

This project is licensed under the GPL-3.0 License. See the [LICENSE](LICENSE) file for details.
