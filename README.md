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

CPK extends the standard CRUX toolchain to provide a structured way of handling packages and their sources.
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

### **Install and remove packages**  

Install a package directly from internet repositories:  
```sh
cpk install htop
```

Alternatively you can pass the whole version string:
```sh
cpk install htop#3.4.1-1.x86_64.cpk
```

Install local stored package:
```sh
cpk install ~/packages/htop#3.4.1-1.x86_64.cpk
```

Cleanly uninstall a package:
```sh
cpk remove htop
```

### **Build from source**

Build a package from its port:
```sh
cpk build htop
```

The port source tree used to build each version of the package is stored in separate directories under the same `/var/lib/cpk/` directory. For example, sources for `htop` version `3.4.1-1` would be located at: 
```sh
/var/lib/cpk/htop/3.4.1-1/
```

Similarly, if you have another version, such as `htop` version `3.4.0-1`, the source would be stored in:
```sh
/var/lib/cpk/htop/3.4.0-1/
```


### **List or query packages**

```sh
cpk list
cpk info htop
```

## Contributing

If you want to contribute to this project, feel free to fork the repository, make your changes, and submit a pull request. Contributions from the community are always welcome!
See the [CONTRIBUTING](CONTRIBUTING) file for details.

## License

This project is licensed under the GPL-3.0 License. See the [LICENSE](LICENSE) file for details.
