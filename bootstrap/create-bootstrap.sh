#!/usr/bin/env bash

set -e -u
set -o pipefail

download_packages() {
	IFS=$'\n' read -r -d '' -a package_list < <( pactree -ul pacman | sort) || :
	package_list+=(default-keyring)

	# cd $(mktemp -d)
	mkdir -p tmp
	pushd tmp > /dev/null

	local url; for url in $(pacman -Sp "${package_list[@]}"); do
		case "${url}" in
			"oci://"*)
				pkg_name=$(python - <<EOF
import os
import sys
print(os.path.basename("${url}").rsplit('-', maxsplit=3)[0])
EOF
)
				local img_url=$(expac -S 'https://ghcr.io/v2/macos-pacman/core/%n/blobs/sha256:%h' ${pkg_name})
				echo "downloading $(basename ${url})"
				wget --quiet --header 'Authorization: Bearer QQ==' -O $(basename ${url}) ${img_url}
				;;
			"http://"* | "https://"*)
				wget --quiet "${url}"
				;;
			"file://"*)
				cp "${url#file://}" ./
				;;
		esac
	done
	popd > /dev/null
}

download_packages

if [ -e staging ]; then
	rm -rf staging
fi

mkdir -p staging
pushd staging > /dev/null

for f in ../tmp/*.pkg.tar.*; do
	echo "staging $f"
	tar -P -xf "$f"
done

rm .BUILDINFO
rm .PKGINFO
rm .MTREE
tar --strip-components 1 --gid 0 --uid 0 -cf ../bootstrap.tar.xz .

popd > /dev/null

# old_pwd=$PWD
# cd ..
# rm -r $old_pwd
