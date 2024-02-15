#!/usr/bin/env sh

set -e -u
set -o pipefail

msg() {
	printf "\x1b[32;1m==>\x1b[0m \x1b[1m%s\x1b[0m\n" "$@"
}

msg "Overview of bootstrap steps:"
_r=$(tput setaf 1 && tput bold)
_b=$(tput setaf 7 && tput bold)
_g=$(tput setaf 2 && tput bold)
_n=$(tput sgr0)
cat <<EOF
${_b}1.${_n} Download bootstrap archive files from github
${_b}2.${_n} Unpack the archive at ${_g}/opt/pacman/${_n}
${_b}3.${_n} Add ${_g}/opt/pacman/usr/bin/bash${_n} to ${_r}/etc/shells${_n}
${_b}4.${_n} Change your login shell to ${_g}/opt/pacman/usr/bin/bash${_n}
${_b}5.${_n} Ensure ${_g}/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk${_n} points to the correct major version
   ${_b}a)${_n} Then delete all other irrelevant SDK versions
${_b}6.${_n} Initialise Pacman
   ${_b}a)${_n} Set up Pacman keyrings
   ${_b}b)${_n} Synchronise packages (via ${_g}pacman -Syu${_n})
EOF

read -p "Continue (y/n)? " choice
case "$choice" in
  y|Y*) ;;
  *) msg "Cancelling!"; exit ;;
esac

# cd /tmp
rm -f bootstrap.tar.xz
msg "Downloading bootstrap archive"

curl -fSL --progress-bar https://github.com/macos-pacman/core/releases/download/system-bootstrap-$(uname -m)/bootstrap.tar.xz \
	> bootstrap.tar.xz

msg "sha256: $(shasum -a 256 bootstrap.tar.xz | cut -wf1)"

msg "Setting up directories (might request root password)"
sudo mkdir -p /opt/pacman

msg "Unpacking archive"
sudo tar xpf bootstrap.tar.xz -C /

msg "Adding /opt/pacman/usr/bin/bash to /etc/shells"
echo "/opt/pacman/usr/bin/bash" | sudo tee -a /etc/shells

msg "Changing user's login shell"
sudo chsh -s /opt/pacman/usr/bin/bash $(whoami)


pushd '/Library/Developer/CommandLineTools/SDKs/' > /dev/null

major_ver=$(sw_vers -productVersion | cut -d. -f1)
msg "Linking MacOSX.sdk -> MacOSX${major_ver}.sdk"

tmp=$(readlink "MacOSX${major_ver}.sdk")

sudo rm -f "MacOSX.sdk"
sudo ln -vsf "${tmp}" "MacOSX.sdk"

# even if we do this, `xcrun --show-sdk-path` is a motherfucker and somehow hunts down MacOSX14.2.sdk.
# if we delete the 14 sdk folders, it seems to relent. This'll probably hold for 15 etc. down the line.
for sdk in *; do
    # if the major version is a mismatch, yeet it. (no need to remove `.sdk` when matching)
    tmp=$(echo "${sdk#MacOSX}" | cut -d. -f1)
    if [[ "${tmp}" != "${major_ver}" ]] && [[ "${sdk}" != "MacOSX.sdk" ]]; then
        msg "  * Deleting ${sdk}"
        sudo rm -f "${sdk}"
    fi
done
popd > /dev/null


# relaunch a shell, and do the rest
cat <<'EOF' | /opt/pacman/usr/bin/bash -
set -e -u
msg() {
    printf "\x1b[32;1m==>\x1b[0m \x1b[1m%s\x1b[0m\n" "$@"
}

msg "Initialising pacman key"
export PATH="/opt/pacman/usr/bin/:$PATH"
sudo pacman-key --init
sudo pacman-key --populate

msg "Synchronising Pacman repositories"
sudo pacman -Syu --overwrite='/*' --noconfirm

msg "Re-installing base packages"
sudo pacman -S --overwrite='/*' --noconfirm pacman-contrib default-keyring sed $(pactree -uls pacman | sort | paste -sd' ' -)

msg "Done"
EOF





