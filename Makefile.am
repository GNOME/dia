## Process this file with automake to produce Makefile.in

SUBDIRS = lib objects plug-ins shapes app samples po sheets doc

gnomedatadir = $(datadir)

pkgdata_DATA = dia_logo.png

pixmapdir = $(gnomedatadir)/pixmaps
pixmap_DATA = dia_gnome_icon.png dia-diagram.png

Applicationsdir = $(gnomedatadir)/applications
applications_in_files = dia.desktop.in
Applications_DATA = $(applications_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

dia.keys: $(srcdir)/dia.keys.in Makefile
	sed -e 's^\@icondir\@^$(pixmapdir)^g' $(srcdir)/dia.keys.in > dia.keys.tmp \
	&& mv dia.keys.tmp dia.keys

CLEANFILES = core-translation-report dia.keys dia.desktop

distclean-local:
	rm -f dia.keys
	rm -f intltool-extract
	rm -f intltool-merge
	rm -f intltool-update

mimedir = $(gnomedatadir)/mime-info
mime_DATA = dia.mime dia.keys

po/dia.pot: 
	(cd po ; $(MAKE) dia.pot)

po/Makefile: po/Makefile.in
	(cd po ; make -f Makefile.in Makefile ; make) # gettext hack

core-translation-report: po/dia.pot po-checktrans.py po-checktrans
	-(cd $(srcdir) && ./po-checktrans po/dia.pot po/*.po)

all-local: core-translation-report po/Makefile

# on uninstall scrollkeeper will leave behind an empty database
# See http://mail.gnome.org/archives/desktop-devel-list/2003-July/msg00144.html
# Requires automake-1.7, so 'export WANT_AUTOMAKE=1.7' before './autogen.sh'
distuninstallcheck_listfiles = find . -type f -print | grep -v '^\./var/scrollkeeper'

#distuninstallcheck:
#	@:

EXTRA_DIST = dia.xpm dia_gnome_icon.png dia_gnome_menu_icon.png \
	dia.desktop.in KNOWN_BUGS $(pkgdata_DATA) \
	dia-diagram.png dia.mime dia.keys.in \
	po-checktrans.py po-checktrans \
	intltool-extract.in intltool-merge.in intltool-update.in \
	RELEASE-PROCESS autogen.sh makefile.msc \
	config.h.win32 readme.win32 AUTHORS MAINTAINERS THANKS

if MAINTAINER_MODE
THANKS: all-recursive
	app/run_dia.sh --credits > THANKS
endif
