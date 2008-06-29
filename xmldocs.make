#
# No modifications of this Makefile should be necessary.
#
# To use this template:
#     1) Define: figdir, docname, lang, omffile, and entities in
#        your Makefile.am file for each document directory,
#        although figdir, omffile, and entities may be empty
#     2) Make sure the Makefile in (1) also includes 
#        "include $(top_srcdir)/xmldocs.make" and
#        "dist-hook: app-dist-hook".
#     3) Optionally define 'entities' to hold xml entities which
#        you would also like installed
#     4) Figures must go under $(figdir)/ and be in PNG format
#     5) You should only have one document per directory 
#     6) Note that the figure directory, $(figdir)/, should not have its
#        own Makefile since this Makefile installs those figures.
#
# example Makefile.am:
#   figdir = figures
#   docname = scrollkeeper-manual
#   lang = C
#   omffile=scrollkeeper-manual-C.omf
#   entities = fdl.xml
#   include $(top_srcdir)/xmldocs.make
#   dist-hook: app-dist-hook
#
# About this file:
#       This file was taken from scrollkeeper_example2, a package illustrating
#       how to install documentation and OMF files for use with ScrollKeeper 
#       0.3.x and 0.4.x.  For more information, see:
#               http://scrollkeeper.sourceforge.net/
#       Version: 0.1.2 (last updated: March 20, 2002)
#


# **********  Begin of section some packagers may need to modify  **********
# This variable (helpdocdir) specifies where the documents should be installed.
# This default value should work for most packages.
if HAVE_GNOME
helpdocdir = $(datadir)/gnome/help/$(docname)/$(lang)
install-data-hook: install-data-hook-omf
else
helpdocdir = $(datadir)/$(docname)/help/$(lang)
install-data-hook:
endif

# **********  You should not have to edit below this line  **********
xml_files = $(entities) $(docname).xml

EXTRA_DIST = $(xml_files) $(omffile)
CLEANFILES = omf_timestamp

include $(top_srcdir)/omf.make
include $(top_srcdir)/hardcopies.make

$(docname).xml: $(entities)
	-ourdir=`pwd`;  \
	cd $(srcdir);   \
	cp $(entities) $$ourdir

app-dist-hook:
	if test "$(figdir)"; then \
	  $(mkinstalldirs) $(distdir)/$(figdir); \
	  for file in $(srcdir)/$(figdir)/*.png; do \
	    basefile=`echo $$file | sed -e  's,^.*/,,'`; \
	    $(INSTALL_DATA) $$file $(distdir)/$(figdir)/$$basefile; \
	  done \
	fi

install-data-xml: omf
	$(mkinstalldirs) $(DESTDIR)$(helpdocdir)
	for file in $(xml_files); do \
	  cp $(srcdir)/$$file $(DESTDIR)$(helpdocdir); \
	done
	if test "$(figdir)"; then \
	  $(mkinstalldirs) $(DESTDIR)$(helpdocdir)/$(figdir); \
	  for file in $(srcdir)/$(figdir)/*.png; do \
	    basefile=`echo $$file | sed -e  's,^.*/,,'`; \
	    $(INSTALL_DATA) $$file $(DESTDIR)$(helpdocdir)/$(figdir)/$$basefile; \
	  done \
	fi

install-data-html: $(progname)_html
	$(mkinstalldirs) $(DESTDIR)$(helpdocdir)
	cp -r $(srcdir)/$(progname)_html/* $(DESTDIR)$(helpdocdir)

uninstall-local-xml: uninstall-local-doc uninstall-local-omf

uninstall-local-html:
	-rm -f $(DESTDIR)$(helpdocdir)/*.html
	-rm -f $(DESTDIR)$(helpdocdir)/$(figdir)/*.png
	-rmdir $(DESTDIR)$(helpdocdir)/$(figdir)
	-rm -f $(DESTDIR)$(helpdocdir)/images/callouts/*.png
	-rmdir $(DESTDIR)$(helpdocdir)/images/callouts
	-rm -f $(DESTDIR)$(helpdocdir)/images/*.png
	-rmdir $(DESTDIR)$(helpdocdir)/images/
	-rm -f $(DESTDIR)$(helpdocdir)/css/*.css
	-rmdir $(DESTDIR)$(helpdocdir)/css
	-rmdir $(DESTDIR)$(helpdocdir)/*
	-rmdir $(DESTDIR)$(helpdocdir)

uninstall-local-doc:
	-rm -f $(DESTDIR)$(helpdocdir)/$(figdir)/*
	-rmdir $(DESTDIR)$(helpdocdir)/$(figdir)
	-rm -f $(DESTDIR)$(helpdocdir)/*
	-rmdir $(DESTDIR)$(helpdocdir)/*
	-rmdir $(DESTDIR)$(helpdocdir)

clean-local-xml: clean-local-omf
