#!/usr/bin/env sh

RUN_DIA=$1

if [ ! -f "$RUN_DIA" ]; then
    echo Please specify run_dia.sh path.
    exit 1
fi

FAILED=0
TEMP_DIR=$(mktemp -d)

FILTER=$2
for FILE in *.dia; do
    DIA_BASENAME=$(basename "${FILE}" .dia)
    OUTPUT=${TEMP_DIR}/${DIA_BASENAME}.${FILTER}
    EXPECTED_OUTPUT=${FILTER}/${DIA_BASENAME}.${FILTER}
    if ! ${RUN_DIA} -t ${FILTER} -e ${OUTPUT} ${FILE}; then
        echo ${FILTER} failed for ${FILE};
        rm -r ${TEMP_DIR}
        exit 1
    elif [ ! -f ${OUTPUT} ]; then
        echo Unable to find output file ${OUTPUT}
        rm -r ${TEMP_DIR}
        exit 2
    elif ! diff -q ${OUTPUT} ${EXPECTED_OUTPUT}; then
        echo "FAILED: ${FILTER} output different from expected for ${FILE}"
        #TODO: currently this fails almost all testcases.  Why?
        FAILED=3
    fi;
done;

rm -r ${TEMP_DIR}

exit ${FAILED}
