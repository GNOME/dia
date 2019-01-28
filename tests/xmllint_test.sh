#!/usr/bin/env sh
DIA=$1
SHAPE_DTD=$2
DIAGRAM=$3

FAILED=0

set -x

# TODO: can we use mktemp instead of rt.shape?
${DIA} ${DIAGRAM} --export=rt.shape || exit 1
xmllint --dtdvalid ${SHAPE_DTD} rt.shape || exit 2
rm -f rt.shape

exit 0
