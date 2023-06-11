# macOS / Pacman

A collection of PKGBUILDs to build packages on macOS.


## Using pacman

The easiest way to use this repository is to build pacman using the provided patchset, and add the following
section to your `pacman.conf` (already done in the installed config):

```
[core]
Server = oci://github.com/macos-pacman/$repo/releases/download/pkg-db-$arch/
```

The patches allow pacman to download package binaries from OCI registries using SHA256 digests, with the
database files themselves hosted with GitHub releases. There should be no need to manually download
packages with this method -- everything will just work normally as if it was a normal server.

Currently, only builds for macos-13.0-arm64 are available (release `pkg-db-arm64`).



## PKGBUILD repository

All the PKGBUILDs are under `packages/`, and are generally sourced from upstream Arch Linux. We try to stick
to most of their conventions (eg. split packages for `-docs`) where possible.

If a package is out of date, feel free to open an issue or submit a PR to update the PKGBUILD.


## Bootstrapping

A bootstrap script is provided at `bootstrap/bootstrap.sh`. If [you are lazy](https://0x46.net/thoughts/2019/04/27/piping-curl-to-shell/), you can run the following:

```sh
curl -fsSL https://raw.githubusercontent.com/macos-pacman/core/master/bootstrap/bootstrap.sh | /bin/sh -
```



## License

The license for Arch Linux PKGBUILDs is unclear, but attribution (Maintainer / Contributor comments) has
been maintained in derived files.

The additional custom patches to Pacman are licensed under the Apache 2.0 License.
