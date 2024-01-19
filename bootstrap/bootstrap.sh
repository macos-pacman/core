#!/usr/bin/env sh

set -e -u

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
${_b}5.${_n} Initialise Pacman
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

msg "Setting up directories (will request root)"
sudo mkdir -p /opt/pacman

msg "Unpacking archive"
sudo tar xpf bootstrap.tar.xz -C /

msg "Adding /opt/pacman/usr/bin/bash to /etc/shells"
echo "/opt/pacman/usr/bin/bash" | sudo tee -a /etc/shells

msg "Changing user's login shell"
sudo chsh -s /opt/pacman/usr/bin/bash $(whoami)

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
sudo pacman -Syu

msg "Done"
EOF





