## Process this file with automake to produce Makefile.in

pkglib_LTLIBRARIES = 	libkaos_objects.la
libkaos_objects_la_SOURCES = \
			kaos.h \
			kaos.c \
			goal.c \
			other.c \
			metabinrel.c \
			metaandorrel.c

libkaos_objects_la_LDFLAGS = -export-dynamic -module -avoid-version $(NO_UNDEFINED)

libkaos_objects_la_LIBADD = $(top_builddir)/lib/libdia.la

AM_CPPFLAGS = -I$(top_srcdir)/intl -I$(srcdir)/../../lib \
	$(DEBUG_FLAGS) $(GTK_CFLAGS)

EXTRA_DIST = \
	pixmaps/agent.xpm\
	pixmaps/broken.xpm\
	pixmaps/contributes.xpm\
	pixmaps/mid_controls.xpm\
	pixmaps/mid_monitors.xpm\
	pixmaps/mid_obstructs.xpm \
	pixmaps/goal.xpm
