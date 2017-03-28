docname = dia

SUBDIRS = html en fr eu pl de

EXTRA_DIST = \
	custom-shapes \
	diagram.dtd \
	docbook.xsl \
	shape.dtd \
	sheet.dtd

helpdir = $(datadir)/doc/dia/html

sysdoc = $(DESTDIR)$(docdir)



# a simple rule to symlink en to C
install-data-local: $(distfiles)
	-(cd $(DESTDIR)$(helpdir) && rm -f C && ln -s en C)
	$(mkinstalldirs) $(sysdoc)
	for i in $^; do \
	  if test -f "$$i"; then  \
	    echo "installing $$i" ;\
	    $(INSTALL_DATA) $$i $(sysdoc)/$$(basename $$i); \
	  fi ; \
	done

uninstall-local:
	rm -f $(DESTDIR)$(helpdir)/C
	-rmdir $(DESTDIR)$(helpdir)
	-rmdir $(sysdoc)/*
	-rmdir $(sysdoc)
	
