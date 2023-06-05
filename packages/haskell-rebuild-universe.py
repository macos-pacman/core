#!/usr/bin/env python

import os
import re
import sys
import shutil
import random
import subprocess

from typing import *
from dataclasses import dataclass


@dataclass(eq=True, frozen=True)
class Package:
	name: str
	version: str

	def dependencies(self) -> tuple[list["Package"], list["Package"]]:
		deps = [ parse_package_name(x) for x in run_command(["ghc-pkg", "field", self.name, "depends", "--simple-output"]).split() if "attoparsec-internal" not in x ]

		# for checkdepends, we need to find the PKGBUILD
		# no PKGBUILD = no checkdepends
		checkdeps: list[Package] = []
		try:
			pkgbuild = open(f"/pm/pacman-macos/packages/haskell-{self.name.lower()}/PKGBUILD", "r").read()
			if "checkdepends=(" in pkgbuild:
				tmp = pkgbuild[pkgbuild.index("checkdepends=(") + len("checkdepends=("):]
				tmp2 = [ x.replace("'", "").replace("\"", "") for x in tmp[:tmp.index(')')].strip().split() ]
				for t in tmp2:
					if t.startswith("haskell-"):
						checkdeps.append(Package(name=t[len("haskell-"):], version="0"))

		except FileNotFoundError:
			pass

		return (deps, checkdeps)

	def __str__(self) -> str:
		return f"{self.name}-{self.version}"


def parse_package_name(name: str) -> Package:
	# split by hyphens; if the last component does not contain
	# dots and is exactly 22 characters long, it's the hash.
	# otherwise, no hash, and it's the version number.
	parts: list[str] = name.split('-')

	if ('.' not in parts[-1]) and (21 <= len(parts[-1]) <= 22):
		return Package(name='-'.join(parts[:-2]).lower(), version=parts[-2])
	else:
		return Package(name='-'.join(parts[:-1]).lower(), version=parts[-1])


def run_command(args: list[str], check=False) -> str:
	return subprocess.run(args, capture_output=True, encoding="utf-8", check=check).stdout


def get_broken_package_names() -> set[str]:
	return set(parse_package_name(x).name for x in run_command(["ghc-pkg", "check", "--simple-output"]).split())


def topo_sort_packages(pkgs: dict[str, Package], get_deps: Callable[[str], list[str]]) -> list[str]:
	visited: set[str] = set()
	sorted_pkgs: list[str] = []

	for pkg in pkgs.values():
		if pkg not in visited:
			topo_dfs(pkgs, pkg, visited, sorted_pkgs, get_deps)
	return sorted_pkgs

def topo_dfs(all_pkgs: dict[str, Package], pkg: Package, visited: set[str], sorted: list[str], get_deps: Callable[[str], list[str]]):
	visited.add(pkg.name)
	for dep in get_deps(pkg.name):
		if dep not in visited:
			topo_dfs(all_pkgs, all_pkgs[dep], visited, sorted, get_deps)

	sorted.append(pkg.name)


def get_next_rebuild(sorted_pkgs: list[str], broken: set[str]) -> str:
	assert len(broken) > 0
	for pkg in sorted_pkgs:
		if pkg in broken:
			return pkg

	assert False


def rebuild_package(name: str):
	cwd=f"/pm/pacman-macos/packages/haskell-{name.lower()}"

	# increment pkgrel
	with open(f"{cwd}/PKGBUILD", "rb") as f:
		pkgbuild = f.read()

	with open(f"{cwd}/PKGBUILD", "wb") as f:
		m = re.search(rb"^pkgrel=(\d+)$", pkgbuild, flags=re.M)
		assert m is not None

		pkgrel = int(m.group(1).decode("utf-8"))
		print(f"\x1b[32;1mpkgrel: {pkgrel} -> {pkgrel+1}\x1b[0m")

		tmp = re.sub(rb"^pkgrel=(\d+)$", f"pkgrel={pkgrel+1}".encode("utf-8"), pkgbuild, flags=re.M)
		f.write(tmp)

	env = os.environ
	env["SRCDEST"] = f"{cwd}/src-downloads"

	print("")
	subprocess.check_call([ "makepkg-template", "--template-dir", "/pm/pacman-macos/templates" ], cwd=cwd)
	subprocess.check_call([ "makepkg", "-Ccfi", "--noconfirm", "--nocheck" ], cwd=cwd, env=env)

def check_package(name: str):
	cwd=f"/pm/pacman-macos/packages/haskell-{name.lower()}"
	if not os.path.exists(cwd):
		return

	env = os.environ
	env["SRCDEST"] = f"{cwd}/src-downloads"
	subprocess.check_call([ "makepkg", "-Ccf", "--noarchive" ], cwd=cwd, env=env)

	# after checking, delete downloaded sources
	shutil.rmtree(f"{cwd}/src-downloads")



def do_rebuild_things(all_pkgs: dict[str, Package], sorting: list[str], initial_broken: set[str]):
	if len(initial_broken) == 0:
		print(f"\x1b[32;1mNo broken packages!\x1b[0m")
		return

	broken: set[str] = initial_broken
	while len(broken) > 0:
		print("")
		print(f"\x1b[31;1m{len(broken)} package{'' if len(broken) == 1 else 's'} to go\x1b[0m")

		to_build = get_next_rebuild(sorting, broken)
		s = f"Building: '{all_pkgs[to_build]}'"
		print("")
		print(f"\x1b[35;1m{s}\x1b[0m")
		print(f"\x1b[35;1m{'=' * len(s)}\x1b[0m")
		print("")

		rebuild_package(to_build)

		# refresh brokens for next round
		broken = get_broken_package_names()





def main() -> None:
	all_packages: dict[str, Package] = {}
	for p in [ parse_package_name(x) for x in run_command(["ghc-pkg", "list", "--simple-output" ]).split() ]:
		all_packages[p.name] = p

	print(f"Found {len(all_packages)} packages. Sorting...")

	sorted_pkgs = topo_sort_packages(all_packages, lambda p: [x.name for x in all_packages[p].dependencies()[0]])
	print(f"\n\n")

	broken_pkgs: set[str] = get_broken_package_names()

	# first rebuild all broken packages
	do_rebuild_things(all_packages, sorted_pkgs, broken_pkgs)

	# then check them
	blacklist = set([ "array", "base", "binary", "bytestring", "containers", "deepseq", "directory", "exceptions", "filepath", "ghc", "ghc-bignum", "ghc-boot", "ghc-boot-th",
		"ghc-compact", "ghc-heap", "ghc-prim", "ghci", "haskeline", "hpc", "integer-gmp", "libiserv", "mtl", "parsec", "pretty", "process", "rts", "stm", "template-haskell",
		"terminfo", "text", "time", "transformers", "unix", "xhtml", "cabal", "cabal-syntax", "z-attoparsec-z-attoparsec-internal"
	])

	i: int = 0
	for pkg in broken_pkgs:
		if pkg.lower() in blacklist:
			continue

		i += 1
		print("")
		s = f"Checking: '{pkg}' ({i}/{len(broken_pkgs - blacklist)})"
		print("")
		print(f"\x1b[35;1m{s}\x1b[0m")
		print(f"\x1b[35;1m{'=' * len(s)}\x1b[0m")
		print("")

		check_package(pkg)


if __name__ == "__main__":
	main()



