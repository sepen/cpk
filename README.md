# CRUX Package Keeper (cpk)

CRUX Package Keeper is a toolset designed to efficiently manage packages and their sources in CRUX environments.

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


## Features

- Centralized and standardized package + source management.
- Safe reuse of pre-built packages across different machines.
- Clear metadata about the source and build process of each package.
- Native multi-architecture support with explicit architecture identifiers.


## Relation to pkgutils

`cpk` is not a replacement for `pkgutils` but an extension built on top of it.
All low-level package management tasks (installation, removal, querying, etc.) are still handled by `pkgadd`, `pkgrm`, `pkginfo`, `pkgmk` and related tools from `pkgutils`.

What `cpk` adds on top is:

- A richer package format (`.cpk`) that embeds port sources, metadata, and architecture information.
- Higher-level commands (`cpk search`, `cpk verify`, `cpk diff`, etc.) that extend the functionality of `pkgutils`.
- Improved portability and reproducibility while maintaining full compatibility with the existing CRUX ecosystem.

In short, `pkgutils` does the heavy lifting, while `cpk` provides the missing context and metadata.


## How it works

`cpk` extends the standard CRUX toolchain to provide a structured way of handling packages and their sources.
It consists of three main parts:

- **Binaries** – command-line tools to create, verify, and manage .cpk packages.
- **Configuration** – integrates with existing CRUX tools like ports, prt-get, and pkgutils.
- **Storage** – a dedicated directory `/var/lib/cpk/` where packages and their corresponding sources are stored.


## Basic Usage

```
cpk <command> [options]

Commands:
  update                   Update the index of available packages
  info <package>           Show information about installed or available packages
  search <keyword>         Search for packages by name or keyword
  list                     List all installed packages
  diff                     Show differences between installed and available packages
  verify <package>         Verify integrity of package source files
  build <package>          Build a package from source files
  install <package>        Install or upgrade packages on the system
  uninstall <package>      Remove packages from the system
  upgrade                  Upgrade all installed packages to the latest versions
  clean                    Clean up package source files and temporary directories
  index <repo>             Create a CPKINDEX file for a local repository
  archive <prtdir> <repo>  Create .cpk archive(s) from a directory containing ports
  help                     Show this help message
  version                  Show version information

Options:
  -c, --config <file>      Set an alternative configuration file (default: /etc/cpk.conf)
  -r, --root <path>        Set an alternative installation root (default: /)
  -C, --color              Show colorized output messages
  -v, --verbose            Show verbose output messages
  -h, --help               Print this help information
  -V, --version            Print version information
```

## Command Reference

### `cpk update`

**Usage**: (no arguments)

- Ensures `CPK_HOME_DIR` exists; otherwise prints an error.
- Downloads `CPKINDEX` from `${CPK_REPO_URL}/CPKINDEX` into `CPK_HOME_DIR`.
- Counts available packages and prints the total.

### `cpk info <package>`

**Usage**: one argument (package name or key)

- Finds the package in `CPKINDEX`.
- If not yet downloaded, fetches and extracts the `.cpk` archive.
- Parses the `Pkgfile` and displays fields like Name, Version, Arch, Description, URL, and Dependencies.

### `cpk search <term>`

**Usage**: one argument (substring to match)

- Opens `CPKINDEX` and searches all lines containing the given term.
- Displays all matches in a formatted list, or shows a “no matches found” message.

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

**Usage**: one required argument (package name or key), optional `--upgrade`

- Finds the package in the repository index.
- If already installed:
- Continues only if `--upgrade` is specified; otherwise prints a warning.
- Ensures the package archive exists locally.
- Executes pre-install and post-install scripts when present.
- Runs `pkgadd -r <CPK_INSTALL_ROOT> [ -u ] <package_file>` to install or upgrade.
- Displays the README (if available) after installation.

### `cpk uninstall <package>`

**Usage**: one argument (package name or key)

- Checks if the package is installed; exits if not.
- Executes `pkgrm -r <CPK_INSTALL_ROOT> <pkgname>` to remove it.
- Reports errors or success accordingly.

### `cpk upgrade [pkg ...]`

**Usage**: zero or more package names

- If no arguments: loads all installed packages (bulk upgrade not fully implemented).
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
