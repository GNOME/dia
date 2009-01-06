plugin_sources = wmf.cpp 

if !WITH_LIBEMF
plugin_sources += wmf_gdi.cpp wmf_gdi.h
endif

pkglib_LTLIBRARIES = libwmf_filter.la

libwmf_filter_la_SOURCES = $(plugin_sources)
libwmf_filter_la_LDFLAGS = -export-dynamic -module -avoid-version 
libwmf_filter_la_LIBADD = $(LIBEMF_LIBS)

INCLUDES = -I$(top_srcdir)/intl -I$(top_srcdir)/lib \
  $(DEBUG_FLAGS) $(GTK_CFLAGS) $(LIBEMF_CFLAGS)

EXTRA_DIST = \
	paginate_gdiprint.cpp \
	paginate_gdiprint.h \
	wmf_gdi.cpp \
	wmf_gdi.h \
	libemf-todo.txt

if !WITH_LIBEMF
# Prevent installation
install-pkglibLTLIBRARIES:
	:
endif
