
plugin_sources = \
	dia-render-script.c \
	dia-render-script.h \
	dia-render-script-import.c \
	dia-render-script-object.c \
	dia-render-script-renderer.c \
	dia-render-script-renderer.h

pkglib_LTLIBRARIES = libdrs_filter.la

EXTRA_DIST = \
	dia-render-script.dtd

libdrs_filter_la_SOURCES = $(plugin_sources)

libdrs_filter_la_LDFLAGS = -export-dynamic -module -avoid-version $(NO_UNDEFINED)
libdrs_filter_la_LIBADD = $(CAIRO_LIBS) $(top_builddir)/lib/libdia.la

AM_CPPFLAGS = -I$(srcdir)/../../lib $(GTK_CFLAGS) $(DEBUG_FLAGS)

# Prevent installation - currently a debug tool only
install-pkglibLTLIBRARIES:
	:

