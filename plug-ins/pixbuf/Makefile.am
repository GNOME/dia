## Process this file with automake to produce Makefile.in

pkglib_LTLIBRARIES = libpixbuf_filter.la

libpixbuf_filter_la_SOURCES = \
	pixbuf.c

libpixbuf_filter_la_LDFLAGS = -export-dynamic -module -avoid-version $(NO_UNDEFINED)

libpixbuf_filter_la_LIBADD = $(top_builddir)/lib/libdia.la

AM_CPPFLAGS = -I$(top_srcdir)/intl -I$(srcdir)/../../lib $(GTK_CFLAGS) $(GNOME_CFLAGS) $(DEBUG_FLAGS)
