## Process this file with automake to produce Makefile.in

pkglib_LTLIBRARIES =    libaadl_objects.la
libaadl_objects_la_SOURCES = \
                             aadl.h               \
                             aadl.c		  \
                             aadlbox.c 		  \
                             aadldata.c 	  \
                             aadlport.c           \
                             aadlbus.c 		  \
                             aadldevice.c 	  \
                             aadlmemory.c 	  \
                             aadlprocess.c 	  \
                             aadlprocessor.c 	  \
                             aadlsubprogram.c 	  \
                             aadlsystem.c 	  \
                             aadltext.c           \
                             aadlthread.c 	  \
                             aadlthreadgroup.c    \
                             aadlpackage.c        \
                             edit_port_declaration.h \
                             edit_port_declaration.c

libaadl_objects_la_LDFLAGS = -export-dynamic -module -avoid-version $(NO_UNDEFINED)

libaadl_objects_la_LIBADD = $(top_builddir)/lib/libdia.la

AM_CPPFLAGS = -I$(top_srcdir)/intl -I$(srcdir)/../../lib \
	$(DEBUG_FLAGS) $(GTK_CFLAGS) $(GNOME_CFLAGS) $(PANGOFT2_CFLAGS) $(UNICODE_CFLAGS)

EXTRA_DIST =   pixmaps/aadlbus.xpm                \
               pixmaps/aadldata.xpm		  \
               pixmaps/aadldevice.xpm		  \
               pixmaps/aadlmemory.xpm		  \
               pixmaps/aadlpackage.xpm		  \
               pixmaps/aadlprocessor.xpm	  \
               pixmaps/aadlprocess.xpm		  \
               pixmaps/aadlsubprogram.xpm	  \
               pixmaps/aadlsystem.xpm		  \
               pixmaps/aadlthreadgroup.xpm	  \
               pixmaps/aadlthread.xpm             
