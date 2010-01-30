EXPORT_PLUGINS=cgm dxf eps eps-builtin eps-pango fig mp plt hpgl png png-libart pixbuf-png jpg shape svg tex wpg

tests:
	mkdir /tmp/dia-tests; \
	for FILTER in $(EXPORT_PLUGINS); do \
		mkdir /tmp/dia-tests/$$FILTER; \
		for FILE in *.dia; do \
			echo -n .; \
			OUTPUT=/tmp/dia-tests/$$FILTER/`basename $$FILE .dia`.$$FILTER; \
			ORIGINAL=$$FILTER/`basename $$FILE .dia`.$$FILTER; \
			if ! ../../app/run_dia.sh -t $$FILTER -e $$OUTPUT $$FILE ; then \
				echo; \
				echo $$FILTER failed for $$FILE; \
			elif ! diff $$OUTPUT $$ORIGINAL >/dev/null ; then \
				echo; \
				echo "$$FILTER output has changed for $$FILE (see $$OUTPUT)"; \
			else \
				rm $$OUTPUT; \
			fi; \
		done; \
	done
