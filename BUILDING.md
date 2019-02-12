# Compiling Dia from source

## NOTE FOR WINDOWS USERS

If you are reading this and just want to use Dia rather than
compiling/extending it, you are in the wrong place.  Please go to
http://dia-installer.sourceforge.net and download the precompiled, easily
installable binary.

Otherwise, please read on :)

## Requirements

### General

Dia is currently built using [meson](https://github.com/mesonbuild/meson).  Meson can be installed either via a package manager or run directly from source code.  Please take 2 minutes to read their [Quick Guide](https://mesonbuild.com/Quick-guide.html).  It provides all the needed information to obtain meson and run it.

Build requirements are documented in the root [meson.build](/meson.build) file.  Meson is designed to be readable so please have a skim through the file.  Some dependencies are required, while others are optional.

As can be seen from `meson.build`, the main requirements are **GTK2**, **libxml2** and **zlib**.  Note: at the moment, **intltool** (`intltool-merge`) is also required.

For reference, a number of other libraries are recommended for extra features.  However, not all these have been moved to meson.  It is recommended you run `meson build` (to configure the build directory) and then check which optional dependencies are not found:

- Libxslt allows export through XSLT translation schemas:
  - ftp://ftp.gnome.org/pub/GNOME/sources/libxslt/
- **Python scripting is also possible** by installing Python 2.7 (note that meson requires python 3, therefore python2 needs to be explicitly installed) and pygtk.

### Windows

Currently (as of 1f930b94) the Windows build is under heavy development and not all features are supported.  Help is always welcome!

The following configuration is supported however: MSYS2 (Pacman v5.1.2) + Mingw64 on Windows 10.

It is recommended you develop Dia in a Virtual Machine since Windows 10 is free to download.

For all git-related operations, it is recommended you use https://gitforwindows.org/ instead of MSYS2.

Please read MSYS2 instructions on https://www.msys2.org/ and their wiki carefully before installing MSYS2.  They have three different environments: MSYS2, Mingw64 and Mingw32.  **ALL** packages must be installed from MSYS2 environment, and all development needs to be performed from Mingw64 one.

It is also recommended you use the source version of meson (https://github.com/mesonbuild/meson) rather than using the MSYS2 one.  However, it is still recommended you install meson from `pacman` since it has a few dependencies, most notably `ninja`

It is also recommended you read https://www.gtk.org/download/windows.php, even though it is for GTK3 instead of GTK2.

Assuming you are all set and that pacman is up to date, you'll need to install at least the following packages:
```
base-devel
mingw-w64-x86_64-toolchain
mingw-w64-x86_64-gtk2
mingw-w64-x86_64-python2
mingw-w64-x86_64-python2-pygtk
intltool
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

### 2. Using `run_with_dia_env` (recommended only on *nix)
```
cd path/to/dia
path/to/meson.py build
cd build
ninja
./run_with_dia_env ./app/dia
```

### Running tests

Build dia then simply do `ninja test`.

### Windows issues
The most common issue currently on MSYS2 is that paths are incorrect.  There are two potential types of issues that you need to be aware of:

1. "libdia.dll not found":  This happens because PATH does not contain the directory where libdia.dll is located.  Simply add that directory to PATH, e.g. `PATH=$(pwd)/lib/:$PATH ./run_with_dia_env ./app/dia`
2. "Python site module not found":  This is because PYTHONPATH or PYTHONHOME are not set.  Simply set both to /mingw64/lib/python2.7 and that should resolve the issue.

## Installing

Simply do `ninja install`.
