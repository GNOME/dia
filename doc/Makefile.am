
SUBDIRS = en

EXTRA_DIST = \
	custom-shapes \
	diagram.dtd \
	shape.dtd \
	sheet.dtd

helpdir = $(datadir)/gnome/help/dia

# a simple rule to symlink en to C
install-data-local:
	-(cd $(DESTDIR)$(helpdir) && rm -f C && ln -s en C)
