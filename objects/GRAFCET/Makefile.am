## Process this file with automake to produce Makefile.in

pkglib_LTLIBRARIES = 	libgrafcet_objects.la
libgrafcet_objects_la_SOURCES = \
			grafcet.h \
			grafcet.c \
			step.c  \
			action.c \
			action_text_draw.h \
			action_text_draw.c \
			transition.c \
			boolequation.h \
			boolequation.c \
			condition.c \
			vergent.c \
			vector.c

libgrafcet_objects_la_LDFLAGS = -export-dynamic -module -avoid-version

INCLUDES = -I$(top_srcdir)/intl -I$(srcdir)/../../lib $(GTK_CFLAGS) $(GNOME_CFLAGS) $(XML_CFLAGS)

EXTRA_DIST = \
	pixmaps/etape.xpm \
	pixmaps/action.xpm \
	pixmaps/transition.xpm \
	pixmaps/vergent.xpm \
	pixmaps/vector.xpm \
	pixmaps/condition.xpm




