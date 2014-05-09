if WITH_CAIRO
plugin_sources = \
	diacairo.c \
	diacairo.h \
	diacairo-interactive.c \
	diacairo-renderer.c \
	diacairo-print.c \
	diacairo-print.h 
else
plugin_sources = 
endif

pkglib_LTLIBRARIES = libcairo_filter.la

EXTRA_DIST =

libcairo_filter_la_SOURCES = $(plugin_sources)

libcairo_filter_la_LDFLAGS = -export-dynamic -module -avoid-version $(NO_UNDEFINED)
libcairo_filter_la_LIBADD = $(CAIRO_LIBS) $(top_builddir)/lib/libdia.la

AM_CPPFLAGS = -I$(srcdir)/../../lib $(GTK_CFLAGS) $(DEBUG_FLAGS) $(CAIRO_CFLAGS)
