plugin_sources = pdf.c

if HAVE_POPPLER
plugin_sources += pdf-import.cpp
endif

# Can't do without C++
if HAVE_CPLUSPLUS
pkglib_LTLIBRARIES = libpdf_filter.la
endif

libpdf_filter_la_SOURCES = $(plugin_sources)
libpdf_filter_la_LDFLAGS = -export-dynamic -module -avoid-version $(NO_UNDEFINED)
libpdf_filter_la_LIBADD = $(POPPLER_LIBS) $(top_builddir)/lib/libdia.la

AM_CPPFLAGS = -I$(top_srcdir)/intl -I$(top_srcdir)/lib \
  $(POPPLER_CFLAGS) $(DEBUG_FLAGS) $(GTK_CFLAGS)

# Prevent installation
install-pkglibLTLIBRARIES:
	:

