## Process this file with automake to produce Makefile.in

pkglib_LTLIBRARIES = 	libmisc_objects.la
libmisc_objects_la_SOURCES = \
			libmisc.c \
			analog_clock.c \
			newgroup.c


libmisc_objects_la_LDFLAGS = -export-dynamic -module -avoid-version

INCLUDES = -I$(top_srcdir)/intl -I$(srcdir)/../../lib $(GTK_CFLAGS) $(GNOME_CFLAGS) $(PANGOFT2_CFLAGS) $(UNICODE_CFLAGS)

EXTRA_DIST = \
	pixmaps/analog_clock.xpm \
	pixmaps/newgroup.xpm


