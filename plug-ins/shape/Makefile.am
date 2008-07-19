## Process this file with automake to produce Makefile.in

pkglib_LTLIBRARIES = libshape_filter.la

libshape_filter_la_SOURCES = \
	shape.c \
	shape-export.c

libshape_filter_la_LDFLAGS = -export-dynamic -module -avoid-version

INCLUDES = -I$(top_srcdir)/intl -I$(srcdir)/../../lib \
	$(DEBUG_FLAGS) $(GTK_CFLAGS) $(XML_CFLAGS)

