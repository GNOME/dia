## Process this file with automake to produce Makefile.in

pkglib_LTLIBRARIES = libsvg_filter.la

libsvg_filter_la_SOURCES = svg.c render_svg.c svg-import.c

libsvg_filter_la_LDFLAGS = -export-dynamic -module -avoid-version $(NO_UNDEFINED)

libsvg_filter_la_LIBADD = $(top_builddir)/lib/libdia.la

AM_CPPFLAGS = -I$(top_srcdir)/intl -I$(srcdir)/../../lib \
	$(DEBUG_FLAGS) $(GTK_CFLAGS) $(XML_CFLAGS)
