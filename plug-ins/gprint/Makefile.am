
plugin_sources = \
	diagnomeprint.c \
	diagnomeprintrenderer.c \
	diagnomeprintrenderer.h

if WITH_GNOMEPRINT
pkglib_LTLIBRARIES = libgprint_filter.la
endif

EXTRA_DIST =

libgprint_filter_la_SOURCES = $(plugin_sources)

libgprint_filter_la_LDFLAGS = -export-dynamic -module -avoid-version

libgprint_filter_la_LIBADD = $(GTK_LIBS) $(GNOMEPRINT_LIBS)

INCLUDES = -I$(srcdir)/../../lib $(GTK_CFLAGS) $(DEBUG_FLAGS) $(GNOMEPRINT_CFLAGS)
