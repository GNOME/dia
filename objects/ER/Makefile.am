## Process this file with automake to produce Makefile.in

pkglib_LTLIBRARIES = 	liber_objects.la
liber_objects_la_SOURCES = \
			entity.c \
			relationship.c \
			attribute.c \
			participation.c \
			er.c

liber_objects_la_LDFLAGS = -export-dynamic -module -avoid-version $(NO_UNDEFINED)

liber_objects_la_LIBADD = $(top_builddir)/lib/libdia.la

AM_CPPFLAGS = -I$(top_srcdir)/intl -I$(srcdir)/../../lib \
	$(DEBUG_FLAGS) $(GTK_CFLAGS) $(GNOME_CFLAGS) $(PANGOFT2_CFLAGS) $(UNICODE_CFLAGS)

EXTRA_DIST = \
	pixmaps/attribute.xpm \
	pixmaps/entity.xpm \
	pixmaps/participation.xpm \
	pixmaps/relationship.xpm
