## Process this file with automake to produce Makefile.in

SUBDIRS = lib objects plug-ins shapes intl app samples po sheets doc

gnomedatadir = $(datadir)

pkgdata_DATA = dia_logo.png
man_MANS = dia.1

pixmapdir = $(gnomedatadir)/pixmaps
pixmap_DATA = dia_gnome_icon.png dia-diagram.png

CLEANFILES = core-translation-report

core-translation-report: po/dia.pot po-checktrans.py po-checktrans
	-(cd $(srcdir) && ./po-checktrans po/dia.pot po/*.po)

Applicationsdir = $(gnomedatadir)/gnome/apps/Applications
Applications_DATA = dia.desktop 

dia.keys: $(srcdir)/dia.keys.in Makefile
	sed -e 's^\@icondir\@^$(pixmapdir)^g' $(srcdir)/dia.keys.in > dia.keys.tmp \
	&& mv dia.keys.tmp dia.keys

distclean-local:
	rm -f dia.keys

mimedir = $(gnomedatadir)/mime-info
mime_DATA = dia.mime dia.keys

po/Makefile: po/Makefile.in
	(cd po ; make -f Makefile.in Makefile ; make) # gettext hack

all-local: core-translation-report po/Makefile

EXTRA_DIST = dia.xpm dia_gnome_icon.png dia_gnome_menu_icon.png \
	dia.desktop dia.spec KNOWN_BUGS $(pkgdata_DATA) $(man_MANS) \
	dia-diagram.png dia.mime dia.keys.in \
	core-translation-report po-checktrans.py po-checktrans


