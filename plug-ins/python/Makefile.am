INCLUDES = \
	-I$(top_srcdir)/intl \
	-I$(top_srcdir)/lib \
	-I$(GTK_CFLAGS) \
	$(GNOME_CFLAGS) \
	$(LIBART_CFLAGS) \
	$(PYTHON_INCLUDES)

if WITH_PYTHON
libpython_plugin = libpython_plugin.la
endif

pkglib_LTLIBRARIES = $(libpython_plugin)

libpython_plugin_la_LDFLAGS = -export-dynamic -module -avoid-version
libpython_plugin_la_LIBADD = $(PYTHON_LIBS)
libpython_plugin_la_SOURCES = \
	pydia-color.c \
	pydia-color.h \
	pydia-cpoint.c \
	pydia-cpoint.h \
	pydia-diagram.c \
	pydia-diagram.h \
	pydia-diagramdata.c \
	pydia-diagramdata.h \
	pydia-display.c \
	pydia-display.h \
	pydia-error.c \
	pydia-error.h \
	pydia-export.c \
	pydia-export.h \
	pydia-font.c \
	pydia-font.h \
	pydia-geometry.c \
	pydia-geometry.h \
	pydia-handle.c \
	pydia-handle.h \
	pydia-image.c \
	pydia-image.h \
	pydia-layer.c \
	pydia-layer.h \
	pydia-object.c \
	pydia-object.h \
	pydia-properties.c \
	pydia-properties.h \
	pydia-property.c \
	pydia-render.c \
	diamodule.c \
	python.c

if WITH_PYTHON

pkgdata_DATA = python-startup.py

pyplugindir = $(pkgdatadir)/python
pyplugin_DATA = \
	gtkcons.py
## are the other python plugins good for more than just testing purposes?

endif

EXTRA_DATA = \
	python-startup.py \
	debug_objects.py \
	export-object.py \
	export-render.py \
	gtkcons.py
