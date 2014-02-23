## Process this file with automake to produce Makefile.in

pkglib_LTLIBRARIES = 	libistar_objects.la
libistar_objects_la_SOURCES = \
			istar.h \
			istar.c \
			actor.c \
			goal.c \
			other.c \
			link.c

libistar_objects_la_LDFLAGS = -export-dynamic -module -avoid-version $(NO_UNDEFINED)

libistar_objects_la_LIBADD = $(top_builddir)/lib/libdia.la

AM_CPPFLAGS = -I$(top_srcdir)/intl -I$(srcdir)/../../lib \
	$(DEBUG_FLAGS) $(GTK_CFLAGS)

EXTRA_DIST = \
	pixmaps/actor.xpm \
	pixmaps/softgoal.xpm \
	pixmaps/resource.xpm \
	pixmaps/link.xpm
