
helpdir = $(datadir)/gnome/help/dia/en

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
	usage-objects-basic.sgml \
	usage-objects-selecting.sgml \
	usage-objects-special.sgml \
	usage-objects.sgml \
	usage-quickstart.sgml

EXTRA_DIST = \
	topic.dat \
	graphics/greendots.png \
	$(SGML_FILES)

if HAVE_DB2HTML
stamp-doc: $(SGML_FILES)
	cd $(srcdir) && $(DB2HTML) dia-manual.sgml && touch stamp-doc
else
stamp-doc:
	touch stamp-doc
endif

# dist and install the html documentation ...
dist-hook:
	mkdir $(distdir)/dia-manual
	-cp $(srcdir)/dia-manual/*.html $(srcdir)/dia-manual/*.css $(distdir)/dia-manual
	mkdir $(distdir)/dia-manual/stylesheet-images
	-cp $(srcdir)/dia-manual/stylesheet-images/*.gif $(distdir)/dia-manual/stylesheet-images

install-data-hook: stamp-doc
	$(mkinstalldirs) $(DESTDIR)$(helpdir)
	for file in $(srcdir)/dia-manual/*.html $(srcdir)/dia-manual/*.css; do\
	  $(INSTALL_DATA) $$file $(DESTDIR)$(helpdir)/`basename $$file`; \
	done
	$(mkinstalldirs) $(DESTDIR)$(helpdir)/stylesheet-images
	for file in $(srcdir)/dia-manual/stylesheet-images/*.gif; do \
	  $(INSTALL_DATA) $$file $(DESTDIR)$(helpdir)/stylesheet-images/`basename $$file`; \
	done
	$(INSTALL_DATA) $(srcdir)/graphics/greendots.png $(DESTDIR)$(helpdir)

