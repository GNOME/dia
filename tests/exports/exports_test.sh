#!/usr/bin/env sh

EXPORT_PLUGINS="cgm dxf eps eps-builtin eps-pango fig mp plt hpgl png png-libart pixbuf-png jpg shape svg tex wpg"

RUN_DIA=$1

if [ ! -f "$RUN_DIA" ]; then
    echo Please specify run_dia.sh path.
    exit 1
fi

FAILED=0
TEMP_DIR=$(mktemp -d)
for FILTER in ${EXPORT_PLUGINS}; do
	for FILE in *.dia; do
        DIA_BASENAME=$(basename "${FILE}" .dia)
		OUTPUT=${TEMP_DIR}/${DIA_BASENAME}.${FILTER}
		EXPECTED_OUTPUT=${FILTER}/${DIA_BASENAME}.${FILTER}
		if ! ${RUN_DIA} -t ${FILTER} -e ${OUTPUT} ${FILE}; then
			echo ${FILTER} failed for ${FILE};
            exit 1
		elif ! diff -q ${OUTPUT} ${EXPECTED_OUTPUT}; then
			echo "FAILED: ${FILTER} output different from expected for ${FILE}" 
            #TODO: currently this fails almost all testcases.  Why?
            FAILED=1
		fi;
	done;
done

rm -r ${TEMP_DIR}

exit ${FAILED}
