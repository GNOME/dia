
if HAVE_CAIRO
plugin_sources = diacairo.c
plugin_extra_dist =
else
plugin_sources = 
plugin_extra_dist = diacairo.c
endif

pkglib_LTLIBRARIES = libcairo_filter.la

EXTRA_DIST = $(plugin_extra_dist)

libcairo_filter_la_SOURCES = $(plugin_sources)

libcairo_filter_la_LDFLAGS = -export-dynamic -module -avoid-version

libcairo_filter_la_LIBADD = $(CAIRO_LIBS)

INCLUDES = -I$(srcdir)/../../lib $(CAIRO_CFLAGS)
