#!/usr/bin/env sh
if [ ! -d "${MESON_SOURCE_ROOT}" ]; then
    echo "Please supply MESON_SOURCE_ROOT variable"
    exit 1
fi
if [ ! -d "${MESON_BUILD_ROOT}" ]; then
    echo "Please supply MESON_BUILD_ROOT variable"
    exit 2
fi
export DIA_BASE_PATH="${MESON_SOURCE_ROOT}"
export DIA_LIB_PATH="${MESON_BUILD_ROOT}/objects/:${MESON_BUILD_ROOT}/plug-ins"
export DIA_SHAPE_PATH="${MESON_SOURCE_ROOT}/shapes/"
export DIA_SHEET_PATH="${MESON_BUILD_ROOT}/sheets"
export DIA_XSLT_PATH="${MESON_SOURCE_ROOT}/plug-ins/xslt/"
export DIA_PYTHON_PATH="${MESON_SOURCE_ROOT}/plug-ins/python/"
${MESON_BUILD_ROOT}/app/dia "$@"
