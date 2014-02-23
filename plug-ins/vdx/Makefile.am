## Process this file with automake to produce Makefile.in

pkglib_LTLIBRARIES = libvdx_filter.la

libvdx_filter_la_SOURCES = \
	vdx.c \
	visio-types.h \
	vdx-import.c \
	vdx-export.c \
	vdx-common.c \
	vdx-xml.c \
	vdx.h

libvdx_filter_la_LDFLAGS = -export-dynamic -module -avoid-version $(NO_UNDEFINED)

libvdx_filter_la_LIBADD = $(top_builddir)/lib/libdia.la

AM_CPPFLAGS = -I$(top_srcdir)/intl -I$(srcdir)/../../lib $(GTK_CFLAGS) $(GNOME_CFLAGS) $(PANGOFT2_CFLAGS) $(XML_CFLAGS) $(UNICODE_CFLAGS) $(DEBUG_FLAGS)
