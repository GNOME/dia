# Compiling Dia from source

## Requirements

### General

Dia is built using [meson](https://github.com/mesonbuild/meson), take a look
at their [Quick Guide](https://mesonbuild.com/Quick-guide.html) for help
getting started with meson.

We have a number of direct dependencies, check the `dependency()` calls in
the root [meson.build](/meson.build) file for the definitive list. Note some
dependencies are only needed when building certain plug-ins.

Our key dependencies are Gtk 3, GLib/GIO, libxml2, cairo, and graphene.

Some optional dependencies include libxslt, for the XSLT translation plug-in,
and python3/PyGObject for python scripting.

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
