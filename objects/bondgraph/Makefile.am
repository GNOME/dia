## Process this file with automake to produce Makefile.in

pkglib_LTLIBRARIES = 	libbondgraph_objects.la
libbondgraph_objects_la_SOURCES = \
			bondgraph.c

libbondgraph_objects_la_LDFLAGS = -export-dynamic -module -avoid-version

INCLUDES = -I$(top_srcdir)/intl -I$(srcdir)/../../lib $(GTK_CFLAGS) \
	$(GNOME_CFLAGS) $(PANGOFT2_CFLAGS) $(UNICODE_CFLAGS)

EXTRA_DIST = \
	pixmaps/bondgraph.xpm 
