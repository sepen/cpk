# CRUX Package Keeper (cpk)

CRUX Package Keeper is a toolset designed to efficiently manage packages and their sources in CRUX environments.

## Motivation

While maintaining several CRUX installations, I ran into some recurring issues:

- **Multiple installations** - I often had to rebuild packages from scratch on each machine instead of reusing already compiled ones.

- **Lack of source traceability** - After generating many `.pkg.tar.*` files across systems, it became difficult to know which source and/or version was used to build each package.

- **Multi-architecture support** - Working with different architectures (x86_64, arm64, armhf, etc.), I noticed that the `.pkg.tar.*` format does not provide a clear way to distinguish between them, making package management more complex.

To solve these challenges in a practical way, I created **CRUX Package Keeper (cpk)**. The `.cpk` format addresses the problem of not knowing which source was used to build a package by combining the `.pkg.tar.xz` file with the source tree used at the time of compilation. Additionally, it includes a signature to verify the integrity of the source. This way, I can store and reuse the same packages across different machines without losing track of their origins or integrity.


## Features

- Centralize and standardize package and source management.

- Enable safe reuse of pre-built packages across different machines.

- Maintain clear metadata about the source of each package.

- Provide native multi-architecture support.


## How it works

CPK extends the standard CRUX toolchain to provide a structured way of handling packages and their sources.
It consists of three main parts:

- **Binaries** – command-line tools to create, verify, and manage .cpk packages.

- **Configuration** – integrates with existing CRUX tools like ports, prt-get, and pkgutils.

- **Storage** – a dedicated directory `/var/lib/cpk/` where packages and their corresponding sources are stored.


## Basic Usage

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
