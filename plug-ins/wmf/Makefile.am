plugin_sources = wmf.cpp wmf_gdi.cpp wmf_gdi.h
pkglib_LTLIBRARIES = libwmf_filter.la

libwmf_filter_la_SOURCES = $(plugin_sources)
libwmf_filter_la_LDFLAGS = -export-dynamic -module -avoid-version 
libwmf_filter_la_LIBADD = -lstdc++

INCLUDES = -I$(top_srcdir)/intl -I$(top_srcdir)/lib \
  $(GTK_CFLAGS) $(GNOME_CFLAGS) $(PANGOFT2_CFLAGS) $(UNICODE_CFLAGS) \
  $(DEBUG_CFLAGS)

EXTRA_DIST = 

# Prevent installation
install-pkglibLTLIBRARIES:
	:
