set(CMAKE_INSTALL_PREFIX "/opt/pacman/usr")
set(CMAKE_MACOSX_RPATH FALSE)
set(CMAKE_INSTALL_NAME_DIR "/opt/pacman/usr/lib")

# llvm fucks us up
set(CMAKE_AR "/usr/bin/ar")
set(CMAKE_RANLIB "/usr/bin/ranlib")
