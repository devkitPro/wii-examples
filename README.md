# Wii Examples With CMake!

I've taken the original Wii examples from DevkitPPC and I've added in CMakeLists.txts which references my DevkitPPC cmake toolchain in order to add more simple build system construction, as well as cross platform IDE support. 

### Download the sources

In order to keep the toolchain repository nice and light, we don't use the add the examples into the toolchain, we add the toolchain into the examples, and we do that with submodules. So in order to build the wii examples, first pull the repo and all it's submodules

```bash
git clone --recurse-submodules https://github.com/LawG4/wii-examples.git
```

or you can separate the git clone step and the submodule pulling section

```bash
git clone https://github.com/LawG4/wii-examples.git
cd wii-examples
git submodule update --init --recursive .
```

### Build

```bash
# Make a directory for CMake to place the build system into
cd wii-examples
mkdir build
cd build

# Configure the CMake build system 
cmake .. -GNinja -DCMAKE_TOOLCHAIN_FILE=../DevkitPPC-CMake/DevkitWiiToolchain.cmake

# Launch the build
cmake --build .
```

### Dependencies 

DevkitPPC : https://wiibrew.org/wiki/DevkitPPC
Ninja : https://ninja-build.org
CMake : https://cmake.org/install/
Dolphin : https://dolphin-emu.org