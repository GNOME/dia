
SUBDIRS = en pl

EXTRA_DIST = \
	custom-shapes \
	diagram.dtd \
	shape.dtd \
	sheet.dtd

if HAVE_GNOME
helpdir = $(datadir)/gnome/help/dia
else
helpdir = $(pkgdatadir)/help
endif

# a simple rule to symlink en to C
install-data-local:
	-(cd $(DESTDIR)$(helpdir) && rm -f C && ln -s en C)
uninstall-local:
	rm -f $(DESTDIR)$(helpdir)/C
