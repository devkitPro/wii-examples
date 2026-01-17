# template

This is a cmake template for starting new Wii projects.

To use this you will need cmake installed which can usually be obtained from your system's package manager where available or from https://cmake.org/download/ directly (recommended for macOS). Windows users with msys2 can obtain cmake via `pacman -S cmake`.

For macOS we also recommend adding the cmake binaries to your path for easy command-line access. This can be done by creating a file called cmake in /etc/paths.d with the contents `/Applications/CMake.app/Contents/bin`. You'll need to log out and in again for that to take effect.

We provide a wrapper for cmake which can be invoked directly like this

    /opt/devkitpro/portlibs/wii/bin/powerpc-eabi-cmake -B _wii -S .
    /opt/devkitpro/portlibs/wii/bin/powerpc-eabi-cmake --build _wii


Using the wiivars script adds various toolchain paths so these wrappers can be invoked without the full path

    . /opt/devkitpro/wiivars.sh
    powerpc-eabi-cmake -B _wii -S .
    powerpc-eabi-cmake --build _wii

Or more conventionally we can provide the toolchain file at the configure step.

    cmake -DCMAKE_TOOLCHAIN_FILE=/opt/devkitpro/cmake/Wii.cmake -B _wii -S .
    cmake --build _wii
