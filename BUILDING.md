# Compiling Dia from source

## Requirements

### General

Dia is currently built using [meson](https://github.com/mesonbuild/meson).  Meson can be installed either via a package manager or run directly from source code.  Please take 2 minutes to read their [Quick Guide](https://mesonbuild.com/Quick-guide.html).  It provides all the needed information to obtain meson and run it.

Build requirements are documented in the root [meson.build](/meson.build) file.  Meson is designed to be readable so please have a skim through the file.  Some dependencies are required, while others are optional.

As can be seen from `meson.build`, the main requirements are **GTK3** 3.24, **graphene**, **libxml2** and **zlib**.

For reference, a number of other libraries are recommended for extra features.  However, not all these have been moved to meson.  It is recommended you run `meson build` (to configure the build directory) and then check which optional dependencies are not found:

- Libxslt allows export through XSLT translation schemas:
  - ftp://ftp.gnome.org/pub/GNOME/sources/libxslt/
- **Python scripting is also possible** by installing Python 3 and PyGObject.

### Windows

Currently (as of 1f930b94) the Windows build is under heavy development and not all features are supported.  Help is always welcome!

Please read MSYS2 instructions on https://www.msys2.org/ and their wiki carefully before installing MSYS2.  They have three different environments: MSYS2, Mingw64 and Mingw32.  **ALL** packages must be installed from MSYS2 environment, and all development needs to be performed from Mingw64 one.

Assuming you are all set and that pacman is up to date, you'll need to install at least the following packages:
```
base-devel
mingw-w64-x86_64-toolchain
mingw-w64-x86_64-gtk3
mingw-w64-x86_64-python
```

You might need to install additional packages.  Please file an issue / merge request with the updated packages to ensure others know what to install also.

## Building and Running

Please make sure you at least skim the Meson [Quick Guide](https://mesonbuild.com/Quick-guide.html) before building.

There are currently (as of 1f930b94) two ways to run Dia:

### 1. Installing to a local prefix
```
cd path/to/dia
path/to/meson.py build -Dprefix=`pwd`/build/install
cd build
ninja install
## Wait for meson to configure, build and install
## If missing dependencies, please check "Requirements" section
./install/bin/dia
```

### 2. Using `meson devenv`
```
cd path/to/dia
path/to/meson.py build
cd build
path/to/meson.py devenv
ninja
./app/dia
```

### Running tests

Build dia then simply do `ninja test`.

### Windows issues
The most common issue currently on MSYS2 is that paths are incorrect.  There are two potential types of issues that you need to be aware of:

1. "libdia.dll not found":  This happens because PATH does not contain the directory where libdia.dll is located.  Simply add that directory to PATH, e.g. `PATH=$(pwd)/lib/:$PATH meson devenv ./app/dia`
2. "Python site module not found":  This is because PYTHONPATH or PYTHONHOME are not set.  Simply set both to /mingw64/lib/python3.<x> and that should resolve the issue.

## Installing

Simply do `ninja install`.
