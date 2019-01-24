#!/usr/bin/env sh

echo "Gathering dependencies.  Please close dia once it has opened."
PYTHONHOME=/mingw64 strace -o strace-log env ${MESON_INSTALL_DESTDIR_PREFIX}/bin/dia.exe

# Search for all .dlls which are loaded from mingw64
grep 'mingw64' strace-log  | awk '{print $5}' | tr '\' '/' | sed 's#C:#/c#' > win-lib-deps

# Filter out those which are in /bin/ and copy them to DESTDIR/bin
grep '/mingw64/bin/' win-lib-deps  | xargs -I{} cp '{}' ${MESON_INSTALL_DESTDIR_PREFIX}/bin/

# Copy the main dependency libraries
cp -r /c/msys64/mingw64/lib/{gtk-2.0,gdk-pixbuf-2.0,python2.7}/ ${MESON_INSTALL_DESTDIR_PREFIX}/lib/


# Cleanup uneeded files.

# No need for static libraries
# NOTE: this removes every .a file, might remove important things.
find ${MESON_INSTALL_DESTDIR_PREFIX}/lib -name '*.a' -delete

# Not needed to run Dia
rm -r ${MESON_INSTALL_DESTDIR_PREFIX}/lib/gtk-2.0/include

# No source distribution or optimized modules.
find ${MESON_INSTALL_DESTDIR_PREFIX}/lib/python2.7/ -name '*.py' -delete
find ${MESON_INSTALL_DESTDIR_PREFIX}/lib/python2.7/ -name '*.pyo' -delete
