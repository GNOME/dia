## Process this file with automake to produce Makefile.in

if WITH_SWIG
libpython_bindings = _dia.la
endif

# the python part (import dia;)
pkgpython_PYTHON = dia.py
pkgpyexec_LTLIBRARIES = $(libpython_bindings)

# c++ wrapper over Dia's C api
_dia_la_SOURCES = \
        dia-connections.cpp \
        dia-connections.h \
	dia-properties.cpp \
	dia-properties.h \
	dia-object.cpp \
	dia-object.h \
	dia-renderer.cpp \
	dia-renderer.h \
	dia-diagramdata.cpp \
	dia-diagramdata.h \
	\
	dia-extra.cpp \
	dia-extra.h \
	\
	dia-python-extra.cpp \
	dia-python-extra.h \
	\
	dia-python.cpp

_dia_la_CPPFLAGS = $(SWIG_PYTHON_CPPFLAGS) -I$(top_srcdir)/lib
_dia_la_LDFLAGS = -export-dynamic -module
_dia_la_LIBADD = ../lib/libdia.la $(PYTHON_LIBS) $(GTK_LIBS)
INCLUDES = \
	-I$(top_srcdir) \
	-I$(top_srcdir)/lib \
	$(GTK_CFLAGS) \
	$(PYTHON_INCLUDES) \
	$(XML_CFLAGS) $(UNICODE_CFLAGS) 

BUILT_SOURCES = \
	dia-python.cpp

# called INTERFACES in makefile.msc, what swig processes
INTERFACES = \
	dia.swig \
	dia-properties.h \
	dia-object.h \
	dia-renderer.h \
	dia-diagramdata.h \
	dia-connections.h

SWIG_PYTHON_OPT = -python -modern -extranative -c++

$(srcdir)/dia-python.cpp : $(INTERFACES)
	$(SWIG) $(SWIG_PYTHON_OPT) -I$(top_srcdir) -I../lib -o $@ $<

EXTRA_DIST = \
	makefile.msc \
	test.py

