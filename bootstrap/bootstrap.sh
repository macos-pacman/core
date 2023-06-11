#!/usr/bin/env sh

set -e -u

msg() {
	printf "\x1b[32;1m==>\x1b[0m \x1b[1m%s\x1b[0m\n" "$@"
}

# cd /tmp
rm -f bootstrap.tar.xz
msg "Downloading bootstrap archive"

curl -fSL --progress-bar https://github.com/macos-pacman/core/releases/download/system-bootstrap-$(uname -m)/bootstrap.tar.xz > bootstrap.tar.xz

msg "Setting up directories (will request root)"
sudo mkdir /opt/pacman

msg "Unpacking archive"
sudo tar xpvf bootstrap.tar.xz -C /

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

msg "Done"
EOF





