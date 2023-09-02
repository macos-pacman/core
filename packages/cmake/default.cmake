set(CMAKE_INSTALL_PREFIX "/opt/pacman/usr")
set(CMAKE_MACOSX_RPATH FALSE)
set(CMAKE_INSTALL_NAME_DIR "/opt/pacman/usr/lib")

# llvm fucks us up

if(NOT CMAKE_AR)
	set(CMAKE_AR "/usr/bin/ar")
endif()

if(NOT CMAKE_RANLIB)
	set(CMAKE_RANLIB "/usr/bin/ranlib")
endif()
