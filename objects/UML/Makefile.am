## Process this file with automake to produce Makefile.in

pkglib_LTLIBRARIES = 	libuml_objects.la
libuml_objects_la_SOURCES = \
			uml.h \
			uml.c \
			class.c \
			class.h \
			class_dialog.c \
			note.c \
			actor.c \
			usecase.c \
			realizes.c \
			constraint.c \
			small_package.c \
			large_package.c \
			implements.c \
			generalization.c \
			association.c \
			dependency.c \
			message.c \
			object.c \
			lifeline.c \
			component.c \
			classicon.c \
			state.c \
			activity.c \
			state_term.c \
			node.c \
			branch.c \
			fork.c \
			stereotype.c \
			stereotype.h

libuml_objects_la_LDFLAGS = -export-dynamic -module -avoid-version

INCLUDES = -I$(top_srcdir)/intl -I$(srcdir)/../../lib $(GTK_CFLAGS) $(GNOME_CFLAGS) $(PANGOFT2_CFLAGS) $(UNICODE_CFLAGS)

EXTRA_DIST = \
	pixmaps/actor.xpm \
	pixmaps/association.xpm \
	pixmaps/case.xpm \
	pixmaps/classicon.xpm \
	pixmaps/component.xpm \
	pixmaps/constraint.xpm \
	pixmaps/dependency.xpm \
	pixmaps/generalization.xpm \
	pixmaps/implements.xpm \
	pixmaps/largepackage.xpm \
	pixmaps/lifeline.xpm \
	pixmaps/message.xpm \
	pixmaps/note.xpm \
	pixmaps/object.xpm \
	pixmaps/realizes.xpm \
	pixmaps/smallpackage.xpm \
	pixmaps/state.xpm \
	pixmaps/umlclass.xpm \
	pixmaps/node.xpm \
	pixmaps/branch.xpm \
	pixmaps/fork.xpm \
	pixmaps/activity.xpm \
	pixmaps/state_term.xpm

