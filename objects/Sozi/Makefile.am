## Process this file with automake to produce Makefile.in

pkglib_LTLIBRARIES = 	libsozi_objects.la
libsozi_objects_la_SOURCES = \
			sozi.c \
			sozi-object.c \
			sozi-object.h \
			sozi-frame.c \
			sozi-media.c \
			sozi-player.h

if WITH_SOZI_PATH
libsozi_objects_la_CFLAGS = -DSOZI_PATH="\"$(SOZI_PATH)\""
endif

libsozi_objects_la_LDFLAGS = -export-dynamic -module -avoid-version $(NO_UNDEFINED)

libsozi_objects_la_LIBADD = $(top_builddir)/lib/libdia.la

AM_CPPFLAGS = -I$(top_srcdir)/intl -I$(srcdir)/../../lib \
	$(DEBUG_FLAGS) $(GTK_CFLAGS)

EXTRA_DIST = \
	pixmaps/sozi-frame.xpm \
	pixmaps/sozi-media.xpm \
	readme.txt



