#!/bin/bash

set -e

# Update everything
pacman --noconfirm -Suy

# Install the required packages
pacman --noconfirm -S --needed \
    base-devel \
    git \
    ${MINGW_PACKAGE_PREFIX}-adwaita-icon-theme \
    ${MINGW_PACKAGE_PREFIX}-appstream \
    ${MINGW_PACKAGE_PREFIX}-atk \
    ${MINGW_PACKAGE_PREFIX}-cairo \
    ${MINGW_PACKAGE_PREFIX}-cc \
    ${MINGW_PACKAGE_PREFIX}-ccache \
    ${MINGW_PACKAGE_PREFIX}-desktop-file-utils \
    ${MINGW_PACKAGE_PREFIX}-gdk-pixbuf2 \
    ${MINGW_PACKAGE_PREFIX}-gettext-tools \
    ${MINGW_PACKAGE_PREFIX}-glib2 \
    ${MINGW_PACKAGE_PREFIX}-gobject-introspection \
    ${MINGW_PACKAGE_PREFIX}-graphene \
    ${MINGW_PACKAGE_PREFIX}-gtk3 \
    ${MINGW_PACKAGE_PREFIX}-meson \
    ${MINGW_PACKAGE_PREFIX}-pango \
    ${MINGW_PACKAGE_PREFIX}-pkgconf \
    ${MINGW_PACKAGE_PREFIX}-python-gobject \
    ${MINGW_PACKAGE_PREFIX}-shared-mime-info \
    ${MINGW_PACKAGE_PREFIX}-toolchain

mkdir -p _ccache
export CCACHE_BASEDIR="$(pwd)"
export CCACHE_DIR="${CCACHE_BASEDIR}/_ccache"

# Build
ccache --zero-stats
ccache --show-stats
export CCACHE_DISABLE=true
meson setup _build -Ddoc=false
unset CCACHE_DISABLE

ninja -C _build
ccache --show-stats
