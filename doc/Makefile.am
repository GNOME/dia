docname = dia

SUBDIRS = en fr eu pl de

EXTRA_DIST = \
	custom-shapes \
	diagram.dtd \
	shape.dtd \
	sheet.dtd

if HAVE_GNOME
helpdir = $(datadir)/gnome/help/$(docname)
else
helpdir = $(pkgdatadir)/help
endif

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
	
