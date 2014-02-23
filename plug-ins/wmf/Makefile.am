plugin_sources = wmf.cpp

if !WITH_LIBEMF
plugin_sources += wmf_gdi.cpp wmf_gdi.h
endif

# Can't do without C++
if HAVE_CPLUSPLUS
pkglib_LTLIBRARIES = libwmf_filter.la
endif

libwmf_filter_la_SOURCES = $(plugin_sources)
libwmf_filter_la_LDFLAGS = -export-dynamic -module -avoid-version $(NO_UNDEFINED)
libwmf_filter_la_LIBADD = $(LIBEMF_LIBS)  $(top_builddir)/lib/libdia.la
if OS_WIN32
libwmf_filter_la_LIBADD += -lcomdlg32
plugin_sources += paginate_gdiprint.cpp paginate_gdiprint.h
endif

AM_CPPFLAGS = -I$(top_srcdir)/intl -I$(top_srcdir)/lib \
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
