## Process this file with automake to produce Makefile.in

pkglib_LTLIBRARIES = 	libsissi_objects.la
libsissi_objects_la_SOURCES = \
			menace.c\
			sissi_dialog.c \
			sissi.h \
			sissi.c \
			sissi_object.c \
			faraday.c \
			room.c\
			site.c\
			area.c

libsissi_objects_la_LDFLAGS = -export-dynamic -module -avoid-version

INCLUDES = -I$(top_srcdir)/intl -I$(srcdir)/../../lib $(GTK_CFLAGS) $(GNOME_CFLAGS) $(FREETYPE_CFLAGS) $(UNICODE_CFLAGS)

EXTRA_DIST = \
	pixmaps/sissi_object.xpm \
	pixmaps/faraday.png  \
	pixmaps/broken.xpm
