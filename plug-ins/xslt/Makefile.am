## Process this file with automake to produce Makefile.in

XSLT_XSL = dia-uml.xsl dia-uml2c++.xsl dia-uml2java.xsl dia-uml2idl.xsl stylesheets
XSLT_SRC = xslt.c xslt.h xsltdialog.c

xsltdir = $(pkgdatadir)/plugins/xslt
xslt_DATA = $(XSLT_XSL)

if HAVE_XSLT	
plugin_sources = $(XSLT_SRC) 
else
plugin_sources = 
endif
plugin_extra_dist=$(XSLT_XSL) $(XSLT_SRC)

EXTRA_DIST = $(plugin_extra_dist)

pkglib_LTLIBRARIES = libxslt_filter.la

libxslt_filter_la_SOURCES = $(plugin_sources)

libxslt_filter_la_LDFLAGS = -export-dynamic -module -avoid-version -ldl -lglib

INCLUDES = -I$(top_srcdir)/intl -I$(srcdir)/../../lib $(GTK_CFLAGS) $(GNOME_CFLAGS) $(PANGOFT2_CFLAGS) $(XML_CFLAGS) $(UNICODE_CFLAGS) $(DEBUG_FLAGS) $(XSLT_CFLAGS)
