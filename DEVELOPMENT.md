## Working with CRUX Containers

### Testing cpk commands

Start a new CRUX container using your preferred container engine (e.g., CONTAINER_ENGINE=podman):
- Mount a volume to persist the working directoy.
```shell
$CONTAINER_ENGINE run -it \
    -v $(pwd):/work \
    docker.io/sepen/crux-multiarch bash
```

Common workflow inside the container:
```shell
# Build and install cpk from sources
cd /work && \
    ./configure && \
    make && make install
install -D -m 0755 /var/lib/cpk
# Run cpk commands
cpk list
cpk update
cpk diff
# and more ... (Run `cpk` help for a list of available commands)
```


### Building ports

Start a new CRUX container using your preferred container engine (e.g., CONTAINER_ENGINE=podman):
- Mount a volume to persist the ports tree.
- Mount another volume to persist the CPK repository files.
```shell
$CONTAINER_ENGINE run -it \
    -v $(pwd)/crux-volume/ports:/usr/ports \
    -v $(pwd)/crux-volume/cpk-repo:/cpk-repo \
    docker.io/sepen/crux-multiarch:3.8 bash
```

Common workflow inside the container:
```shell
# Install cpk from ports
cd /etc/ports && \
    curl -sS -L -O https://raw.githubusercontent.com/sepen/crux-ports-sepen/main/sepen.httpup && \
    cd -
ports -u sepen
prt-get install cpk
# Upgrade system with packages from upstream repository
cpk update
cpk diff
cpk upgrade
# Update port sources and build packages
ports -u
prt-get diff
prt-get sysup
# Archive packages and update repository index
cpk archive /usr/ports /cpk-repo
cpk index /cpk-repo
```

The repository files created inside the container are automatically available on the host system at:
```shell
$(pwd)/crux-volume/cpk-repo
```