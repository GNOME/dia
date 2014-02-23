## Process this file with automake to produce Makefile.in

pkglib_LTLIBRARIES = libpgf_filter.la

libpgf_filter_la_SOURCES = \
	pgf.c \
	render_pgf.c \
	render_pgf.h

libpgf_filter_la_LDFLAGS = -export-dynamic -module -avoid-version $(NO_UNDEFINED)

libpgf_filter_la_LIBADD = $(top_builddir)/lib/libdia.la

AM_CPPFLAGS = -I$(top_srcdir)/intl -I$(srcdir)/../../lib $(DEBUG_FLAGS) $(GTK_CFLAGS)
