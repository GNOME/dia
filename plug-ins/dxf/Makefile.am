## Process this file with automake to produce Makefile.in

pkglib_LTLIBRARIES = libdxf_filter.la

libdxf_filter_la_SOURCES = \
	dxf.c \
	dxf-export.c \
	dxf-import.c

libdxf_filter_la_LDFLAGS = -export-dynamic -module -avoid-version
libdxf_filter_la_LIBADD = @GTK_LIBS@

INCLUDES = -I$(top_srcdir)/intl -I$(srcdir)/../../lib @CFLAGS@ @GTK_CFLAGS@ @GNOME_CFLAGS@
