## Process this file with automake to produce Makefile.in

pkglib_LTLIBRARIES = libdxf_filter.la

libdxf_filter_la_SOURCES = \
	dxf.c \
	dxf-export.c \
	dxf-import.c \
	autocad_pal.c \
	autocad_pal.h

libdxf_filter_la_LDFLAGS = -export-dynamic -module -avoid-version $(NO_UNDEFINED)

libdxf_filter_la_LIBADD = $(top_builddir)/lib/libdia.la

AM_CPPFLAGS = -I$(top_srcdir)/intl -I$(srcdir)/../../lib  $(DEBUG_FLAGS) $(GTK_CFLAGS)
