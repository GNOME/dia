## Process this file with automake to produce Makefile.in

pkglib_LTLIBRARIES = 	libjackson_objects.la
libjackson_objects_la_SOURCES = \
			jackson.h \
			jackson.c \
			domain.c \
			requirement.c \
			phenomenon.c

libjackson_objects_la_LDFLAGS = -export-dynamic -module -avoid-version

INCLUDES = -I$(top_srcdir)/intl -I$(srcdir)/../../lib $(GTK_CFLAGS) $(GNOME_CFLAGS) $(FREETYPE_CFLAGS) $(UNICODE_CFLAGS)

EXTRA_DIST = \
	pixmaps/given_domain.xpm \
	pixmaps/requirement.xpm \
	pixmaps/shared_phen.xpm
