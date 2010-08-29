## Process this file with automake to produce Makefile.in

pkglib_LTLIBRARIES = 	libsissi_objects.la
libsissi_objects_la_SOURCES = \
	menace.c \
	menace.h \
	sissi_dialog.c \
	sissi_dialog.h \
	sissi.c \
	sissi.h \
	sissi_object.c \
	sissi_object.h \
	faraday.c \
	faraday.h \
	room.c \
	room.h \
	site.c \
	site.h \
	area.c \
	area.h

libsissi_objects_la_LDFLAGS = -export-dynamic -module -avoid-version $(NO_UNDEFINED)

libsissi_objects_la_LIBADD = $(top_builddir)/lib/libdia.la

INCLUDES = -I$(top_srcdir)/intl -I$(srcdir)/../../lib \
	$(DEBUG_FLAGS) $(GTK_CFLAGS)

EXTRA_DIST = \
	pixmaps/sissi_object.xpm \
	pixmaps/faraday.png  \
	pixmaps/broken.xpm \
	methode_attack.h \
	classification.h

# Prevent installation - considered too alpha
install-pkglibLTLIBRARIES:
	:

