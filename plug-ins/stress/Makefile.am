## This is currently windows+development only
## It should be easy to port but never get installed
plugin_sources = \
	stress.c \
	stress-memory.c \
	stress-memory.h

pkglib_LTLIBRARIES = libstress_filter.la

libstress_filter_la_SOURCES = $(plugin_sources)
libstress_filter_la_LDFLAGS = -export-dynamic -module -avoid-version $(NO_UNDEFINED)
libstress_filter_la_LIBADD = $(top_builddir)/lib/libdia.la

AM_CPPFLAGS = -I$(top_srcdir)/intl -I$(top_srcdir)/lib \
  $(DEBUG_FLAGS) $(GTK_CFLAGS)

# Prevent installation
install-pkglibLTLIBRARIES:
	:

# see also po/POTFILES.skip

