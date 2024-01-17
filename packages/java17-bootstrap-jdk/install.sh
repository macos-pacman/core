THIS_JDK='java-17-bootstrap'

fix_default() {
  if [ ! -x $(pacman-root-dir)/usr/bin/java ]; then
    $(pacman-root-dir)/usr/bin/archlinux-java unset
    echo ""
  else
    $(pacman-root-dir)/usr/bin/archlinux-java get
  fi
}

post_install() {
  default=$(fix_default)
  case ${default} in
    "")
      $(pacman-root-dir)/usr/bin/archlinux-java set ${THIS_JDK}
      ;;
    ${THIS_JDK})
      # Nothing
      ;;
    *)
      echo "Default Java environment is already set to '${default}'"
      echo "See 'archlinux-java help' to change it"
      ;;
  esac

  if [ ! -f $(pacman-root-dir)/etc/ssl/certs/java/cacerts ]; then
    $(pacman-root-dir)/usr/bin/update-ca-trust
  fi
}

post_upgrade() {
  default=$(fix_default)
  if [ -z "${default}" ]; then
    $(pacman-root-dir)/usr/bin/archlinux-java set ${THIS_JDK}
  fi

  if [ ! -f $(pacman-root-dir)/etc/ssl/certs/java/cacerts ]; then
    $(pacman-root-dir)/usr/bin/update-ca-trust
  fi
}

pre_remove() {
  if [ "x$(fix_default)" = "x${THIS_JDK}" ]; then
    # Check JRE is still available
    if [ -x $(pacman-root-dir)/usr/lib/jvm/${THIS_JDK}/bin/java ]; then
      $(pacman-root-dir)/usr/bin/archlinux-java unset
    fi
  fi
}
