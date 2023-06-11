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
${_b}3.${_n} Edit ${_g}~/.bash_profile${_n} to add /opt/pacman/usr/bin to \$PATH
${_b}4.${_n} Add ${_g}/opt/pacman/usr/bin/bash${_n} to ${_r}/etc/shells${_n}
${_b}5.${_n} Change your login shell to ${_g}/opt/pacman/usr/bin/bash${_n}
${_b}6.${_n} Initialise Pacman
   ${_b}a)${_n} Set up Pacman keyrings
   ${_b}b)${_n} Synchronise packages (via ${_g}pacman -Syu${_n})
EOF

# cd /tmp
rm -f bootstrap.tar.xz
msg "Downloading bootstrap archive"

curl -fSL --progress-bar https://github.com/macos-pacman/core/releases/download/system-bootstrap-$(uname -m)/bootstrap.tar.xz \
	> bootstrap.tar.xz

msg "Setting up directories (will request root)"
sudo mkdir /opt/pacman

msg "Unpacking archive"
sudo tar xpf bootstrap.tar.xz -C /

msg "Setting up \$PATH"
if [ -e $HOME/.bash_profile ]; then
	printf '\x1b[34;1m  ->\x1b[0m \x1b[1mFound existing `.bash_profile`, backing up to `.bash_profile.old`\x1b[0m\n'
	mv $HOME/.bash_profile $HOME/.bash_profile.old
fi

cat <<'EOF' > $HOME/.bash_profile
export PATH=/opt/pacman/usr/bin:$PATH
export CPATH=/opt/pacman/usr/include
export LIBRARY_PATH=/opt/pacman/usr/lib
export MANPATH=/opt/pacman/usr/share/man:$MANPATH

export XDG_CONFIG_DIRS=/opt/pacman/etc/xdg
export XDG_DATA_DIRS=/opt/pacman/usr/share
EOF

msg "Adding /opt/pacman/usr/bin/bash to /etc/shells"
echo "/opt/pacman/usr/bin/bash" | sudo tee -a /etc/shells

msg "Changing user's login shell"
chsh -s /opt/pacman/usr/bin/bash

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





