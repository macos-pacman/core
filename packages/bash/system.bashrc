#
# /etc/bash.bashrc
#

# If not running interactively, don't do anything
[[ $- != *i* ]] && return

[[ $DISPLAY ]] && shopt -s checkwinsize

PS1='[\u@\h \W]\$ '

case ${TERM} in
  Eterm*|alacritty*|aterm*|foot*|gnome*|konsole*|kterm*|putty*|rxvt*|tmux*|xterm*)
    PROMPT_COMMAND+=('printf "\033]0;%s@%s:%s\007" "${USER}" "${HOSTNAME%%.*}" "${PWD/#$HOME/\~}"')

    ;;
  screen*)
    PROMPT_COMMAND+=('printf "\033_%s@%s:%s\033\\" "${USER}" "${HOSTNAME%%.*}" "${PWD/#$HOME/\~}"')
    ;;
esac

if [[ -r /usr/share/bash-completion/bash_completion ]]; then
  . /usr/share/bash-completion/bash_completion
fi

if [[ -r $(pacman-root-dir)/usr/share/bash-completion/bash_completion ]]; then
  . $(pacman-root-dir)/usr/share/bash-completion/bash_completion
fi

export XDG_DATA_DIRS="$(pacman-root-dir)/usr/share:${XDG_DATA_DIRS}"
export XDG_CONFIG_DIRS="$(pacman-root-dir)/etc/xdg:${XDG_CONFIG_DIRS}"
