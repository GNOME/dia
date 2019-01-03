#!/usr/bin/env sh
export DIA_BASE_PATH="${MESON_SOURCE_ROOT}"
export DIA_LIB_PATH="${MESON_BUILD_ROOT}/objects/:${MESON_BUILD_ROOT}/plug-ins"
export DIA_SHAPE_PATH="${MESON_SOURCE_ROOT}/shapes/"
export DIA_SHEET_PATH="${MESON_BUILD_ROOT}/sheets"
export DIA_XSLT_PATH="${MESON_SOURCE_ROOT}/plug-ins/xslt/"
export DIA_PYTHON_PATH="${MESON_SOURCE_ROOT}/plug-ins/python/"
${MESON_BUILD_ROOT}/app/dia
