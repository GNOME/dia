#!/usr/bin/env sh
if [ !  -d "$2" ] || [ ! -d "$3" ]; then
    echo Unable to find both directory $2 and $3.
fi

SOURCE_ROOT=$2
BUILD_ROOT=$3

# Windows has ; for PATH separators.
PATH_SEP=":"
if uname | grep -iq MINGW; then
    PATH_SEP=";"
fi
# This unfortunately is a duplication of run_env in
# tests/meson.build.  This is fine as long as this
# script is only used to generate run_with_dia_env
# and only that.
cat > $1 << EOF
#!/usr/bin/env sh
export DIA_BASE_PATH="${SOURCE_ROOT}"
export DIA_LIB_PATH="${BUILD_ROOT}/objects${PATH_SEP}${BUILD_ROOT}/plug-ins"
export DIA_SHAPE_PATH="${SOURCE_ROOT}/shapes"
export DIA_XSLT_PATH="${SOURCE_ROOT}/plug-ins/xslt"
export DIA_PYTHON_PATH="${SOURCE_ROOT}/plug-ins/python"
export DIA_SHEET_PATH="${BUILD_ROOT}/sheets"
\$@
EOF

chmod u+x $1
