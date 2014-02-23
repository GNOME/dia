
plugin_sources = \
	dialibart.c \
	dialibartrenderer.c \
	dialibartrenderer.h \
	render_libart.h \
	render_libart.c \
	export_png.c 

pkglib_LTLIBRARIES = libart_filter.la

EXTRA_DIST =

libart_filter_la_SOURCES = $(plugin_sources)

libart_filter_la_LDFLAGS = -export-dynamic -module -avoid-version $(NO_UNDEFINED)

libart_filter_la_LIBADD = $(LIBART_LIBS) $(LIBPNG_LIBS) $(top_builddir)/lib/libdia.la

AM_CPPFLAGS = -I$(srcdir)/../../lib $(GTK_CFLAGS) $(DEBUG_FLAGS) $(LIBART_CFLAGS) $(LIBPNG_CFLAGS)
