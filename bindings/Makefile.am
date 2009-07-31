## Process this file with automake to produce Makefile.in

if HAVE_CPLUSPLUS
libpython_bindings = _dia.la
endif

## without SWIG we wont have/build this
## but to avoid bit-rotting we build the rest
if WITH_SWIG
BUILT_SOURCES = \
	dia-python.cpp
endif

## needs just Python
if WITH_PYTHON
PYTHON_EXTRA_SOURCE = \
	dia-python-extra.cpp \
	dia-python-extra.h \
	$(BUILT_SOURCES)
endif

## the rest of the sources is independent of SWIG and Python

## nothing to be installed yet
if WITH_SWIG
## this is not working yet (link error)
#pkgpyexec_LTLIBRARIES = $(libpython_bindings)
# the python part (import dia;)
#pkgpython_PYTHON = dia.py
else
#pkgpython_PYTHON = 
endif
noinst_LTLIBRARIES = $(libpython_bindings)

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
	$(PYTHON_EXTRA_SOURCE)

_dia_la_CPPFLAGS = $(PYTHON_INCLUDES) -I$(top_srcdir)/lib
_dia_la_LDFLAGS = -module -avoid-version -export-symbols-regex init_dia
## must link ../lib/libdia.la but don't know how;(
_dia_la_LIBADD = $(PYTHON_LIBS) $(GTK_LIBS) $(top_builddir)/lib/libdia.la

# trial and error adaption from pygimp;)
#_diadir = $(libdir)/lib
#_dia_LTLIBRARIES = $(top_builddir)/lib/libdia.la

INCLUDES = \
	$(DEBUG_FLAGS) \
	-I$(top_srcdir) \
	-I$(top_srcdir)/lib \
	$(GTK_CFLAGS) \
	$(PYTHON_INCLUDES) \
	$(XML_CFLAGS) $(UNICODE_CFLAGS) 

# called INTERFACES in makefile.msc, what swig processes
INTERFACES = \
	dia.swig \
	dia-properties.h \
	dia-object.h \
	dia-renderer.h \
	dia-diagramdata.h \
	dia-connections.h

SWIG_PYTHON_OPT = -python -modern -extranative -c++

if WITH_SWIG
dia-python.cpp : $(INTERFACES)
	$(SWIG) $(SWIG_PYTHON_OPT) -I$(top_srcdir) -I../lib -o $@ $<
else
dia-python.cpp : $(INTERFACES)
	echo /* No SWIG */ > $@
endif

GCCXMLINC = -I.. -I../lib -I/usr/include/gtk-2.0 -I/usr/lib64/gtk-2.0/include -I/usr/include/atk-1.0 -I/usr/include/cairo -I/usr/include/pango-1.0 -I/usr/include/glib-2.0 -I/usr/lib64/glib-2.0/include -I/usr/include/libxml2 -I/usr/include/libart-2.0   -I/usr/include/python2.5 -I/usr/include/libxml2
$(srcdir)/dia-object.xml : $(INTERFACES)
	gccxml --gccxml-cxxflags '$(GTK_CFLAGS) $(XML_CFLAGS)' -I$(top_srcdir) -I ../lib dia-object.cpp -fxml=$@

EXTRA_DIST = \
	dia.swig \
	makefile.msc \
	Doxyfile \
	test.py

