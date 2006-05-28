SUBDIRS=exports

tests:
	for DIR in $(SUBDIRS); do \
		cd $$DIR && $(MAKE) tests; \
	done
