## Process this file with automake to produce Makefile.in

pkglib_LTLIBRARIES = libxfig_filter.la

libxfig_filter_la_SOURCES = \
	xfig.c \
	xfig-import.c \
	xfig-export.c \
	xfig-common.c \
	xfig.h

libxfig_filter_la_LDFLAGS = -export-dynamic -module -avoid-version $(NO_UNDEFINED)

libxfig_filter_la_LIBADD = $(top_builddir)/lib/libdia.la

AM_CPPFLAGS = -I$(top_srcdir)/intl -I$(srcdir)/../../lib $(GTK_CFLAGS) $(GNOME_CFLAGS) $(PANGOFT2_CFLAGS) $(XML_CFLAGS) $(UNICODE_CFLAGS) $(DEBUG_FLAGS)
