#!/bin/bash

set -e

# Freedesktop doesn't build Poppler with the unstable API enabled, so we build
# it ourselves, but we don't build master since, in danger of stating the
# obvious: the unstable poppler API is unstable, so builds would regularly
# break over changes that may not even end up in a Poppler release.
git clone \
    --depth=1 \
    --branch poppler-26.01.0 \
    https://gitlab.freedesktop.org/poppler/poppler/

# TODO: Can we avoid hardcoding x86_64-linux-gnu ?

mkdir build-poppler
cmake -S poppler -B build-poppler -G Ninja \
    -DCMAKE_INSTALL_PREFIX=/usr \
    -DCMAKE_INSTALL_LIBDIR=/usr/lib/x86_64-linux-gnu/ \
    -DCMAKE_BUILD_TYPE=RelWithDebInfo \
    -DENABLE_UNSTABLE_API_ABI_HEADERS=ON \
    -DENABLE_QT5=OFF \
    -DENABLE_QT6=OFF \
    -DENABLE_BOOST=OFF
ninja -C build-poppler
ninja -C build-poppler install
