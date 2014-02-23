if WITH_FREETYPE
freetype_files = diapsft2renderer.h diapsft2renderer.c
else
freetype_files =
endif

if OS_WIN32
win32_files = win32print.c win32print.h
else
win32_files =
endif

plugin_sources = \
	postscript.c \
	ps-utf8.c \
	ps-utf8.h \
	diapsrenderer.h \
	diapsrenderer.c \
	$(freetype_files) \
	$(win32_files) \
	paginate_psprint.c \
	paginate_psprint.h \
	render_eps.c \
	render_eps.h

pkglib_LTLIBRARIES = libpostscript_filter.la

EXTRA_DIST = \
	diapsft2renderer.c \
	diapsft2renderer.h \
	win32print.c \
	win32print.h

libpostscript_filter_la_SOURCES = $(plugin_sources)

libpostscript_filter_la_LDFLAGS = -export-dynamic -module -avoid-version $(NO_UNDEFINED)

libpostscript_filter_la_LIBADD = $(top_builddir)/lib/libdia.la
if WITH_FREETYPE
libpostscript_filter_la_LIBADD += $(FREETYPE_LIBS)
endif
if OS_WIN32
libpostscript_filter_la_LIBADD += -lwinspool
endif

AM_CPPFLAGS = -I$(srcdir)/../../lib $(GTK_CFLAGS) $(DEBUG_FLAGS) $(CAIRO_CFLAGS)
