## Process this file with automake to produce Makefile.in

pkglib_LTLIBRARIES = 	libeml_objects.la
libeml_objects_la_SOURCES = dbox.h          \
			    dbox.c          \
			    eml.h           \
			    eml.c           \
			    instantiation.c \
			    interaction.h   \
			    interaction.c   \
			    interaction-ortho.c   \
			    listfun.c       \
			    listfun.h       \
			    nlist.h         \
			    nlist.c         \
			    process.h       \
			    process.c       \
			    process_dialog.c





libeml_objects_la_LDFLAGS = -export-dynamic -module -avoid-version

INCLUDES = -I$(top_srcdir)/intl -I$(srcdir)/../../lib $(GTK_CFLAGS) $(GNOME_CFLAGS) $(PANGOFT2_CFLAGS)

EXTRA_DIST = \
	pixmaps/emlprocess.xpm    \
	pixmaps/instantiation.xpm \
	pixmaps/interaction.xpm   \
	pixmaps/interaction-ortho.xpm
