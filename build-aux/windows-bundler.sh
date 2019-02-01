#!/usr/bin/env sh

echo "Gathering dependencies by instrumenting Dia.  Please close once it has fully opened."
PYTHONHOME=/mingw64 strace -o strace-log env ${MESON_INSTALL_DESTDIR_PREFIX}/bin/dia.exe

echo "Done!"

echo "Saved strace log."

# Search for all .dlls which are loaded from mingw64
grep 'mingw64' strace-log  | awk '{print $5}' | sed 's#\\#/#g;s#C:#/c#' > win-lib-deps

echo "Copying all external mingw64 dependencies to ${MESON_INSTALL_DESTDIR_PREFIX}/bin/"
grep '/mingw64/bin/' win-lib-deps  | xargs -r cp -t ${MESON_INSTALL_DESTDIR_PREFIX}/bin/

echo "Copying gtk-2.0 gdk-pixbuf-2.0 and python libraries"
cp -r /c/msys64/mingw64/lib/{gtk-2.0,gdk-pixbuf-2.0,python2.7}/ ${MESON_INSTALL_DESTDIR_PREFIX}/lib/

echo "Copying library translation files (.mo)"

# We want to keep the parent directories when copying.
cd /c/msys64/mingw64/share/locale/
find ./ \( -name 'glib20.mo' -o -name 'gtk20.mo' -o -name 'gtk20-properties.mo' -o -name 'atk10.mo' \) -print0 | xargs -r -0 cp --parents -t ${MESON_INSTALL_DESTDIR_PREFIX}/share/locale/
cd -


echo "Cleaning up uneeded files."

# No need for static libraries
# NOTE: this removes every .a file, might remove important things.
find ${MESON_INSTALL_DESTDIR_PREFIX}/lib -name '*.a' -delete

# Not needed to run Dia
rm -r ${MESON_INSTALL_DESTDIR_PREFIX}/lib/gtk-2.0/include

# No source distribution or optimized modules.
find ${MESON_INSTALL_DESTDIR_PREFIX}/lib/python2.7/ -name '*.py' -delete
find ${MESON_INSTALL_DESTDIR_PREFIX}/lib/python2.7/ -name '*.pyo' -delete

echo "Done!"
