## Process this file with automake to produce Makefile.in

pkglib_LTLIBRARIES = libsvg_filter.la

libsvg_filter_la_SOURCES = svg.c render_svg.c svg-import.c

libsvg_filter_la_LDFLAGS = -export-dynamic -module -avoid-version

INCLUDES = -I$(top_srcdir)/intl -I$(srcdir)/../../lib $(GTK_CFLAGS) $(GNOME_CFLAGS) $(PANGOFT2_CFLAGS) $(UNICODE_CFLAGS) $(XML_CFLAGS) $(DEBUG_CFLAGS)
