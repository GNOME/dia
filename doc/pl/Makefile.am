
if HAVE_GNOME
helpdir = $(datadir)/gnome/help/dia/pl
else
helpdir = $(pkgdatadir)/help/pl
endif

help_DATA = topic.dat

SGML_FILES = \
	dia-manual.sgml \
	authors.sgml \
	intro.sgml \
	license.sgml \
	usage-canvas.sgml \
	usage-customization.sgml \
	usage-layers.sgml \
	usage-loadsave.sgml \
	usage-objects-selecting.sgml \
	usage-objects-special.sgml \
	usage-objects.sgml \
	usage-quickstart.sgml

#	usage-objects-basic.sgml

GRAPHICS = \
	graphics/greendots.png \
	graphics/line_props.png \
	graphics/home_network.png

EXTRA_DIST = \
	topic.dat \
	$(GRAPHICS) \
	$(SGML_FILES)

if HAVE_DB2HTML
stamp-doc: $(SGML_FILES)
	cd $(srcdir) && $(DB2HTML) dia-manual.sgml && touch stamp-doc
else
stamp-doc:
	touch stamp-doc
endif

distclean-local:
	rm -f stamp-doc
	rm -rf $(srcdir)/dia-manual $(srcdir)/dia-manual.junk

# dist and install the html documentation ...
dist-hook:
if HAVE_DB2HTML
	mkdir $(distdir)/dia-manual
	-cp $(srcdir)/dia-manual/*.html $(srcdir)/dia-manual/*.css $(distdir)/dia-manual
	mkdir $(distdir)/dia-manual/stylesheet-images
	-cp $(srcdir)/dia-manual/stylesheet-images/*.gif $(distdir)/dia-manual/stylesheet-images
endif

install-data-hook: stamp-doc
if HAVE_DB2HTML
	$(mkinstalldirs) $(DESTDIR)$(helpdir)
	for file in $(srcdir)/dia-manual/*.html $(srcdir)/dia-manual/*.css; do\
	  $(INSTALL_DATA) $$file $(DESTDIR)$(helpdir)/`basename $$file` || true; \
	done
	$(mkinstalldirs) $(DESTDIR)$(helpdir)/stylesheet-images
	for file in $(srcdir)/dia-manual/stylesheet-images/*.gif; do \
	  $(INSTALL_DATA) $$file $(DESTDIR)$(helpdir)/stylesheet-images/`basename $$file` || true; \
	done
	$(mkinstalldirs) $(DESTDIR)$(helpdir)/graphics
	cd $(srcdir) && for file in $(GRAPHICS); do \
	  $(INSTALL_DATA) $$file $(DESTDIR)$(helpdir)/graphics; \
	done
endif

uninstall-local:
	rm -rf $(DESTDIR)$(helpdir)
