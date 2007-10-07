
plugin_sources = diacairo.c diacairo-ineractive.c diacairo-print.c diacairo.h

pkglib_LTLIBRARIES = libcairo_filter.la

EXTRA_DIST =

libcairo_filter_la_SOURCES = $(plugin_sources)

libcairo_filter_la_LDFLAGS = -export-dynamic -module -avoid-version

libcairo_filter_la_LIBADD = $(CAIRO_LIBS)

INCLUDES = -I$(srcdir)/../../lib $(GTK_CFLAGS) $(DEBUG_FLAGS) $(CAIRO_CFLAGS)
