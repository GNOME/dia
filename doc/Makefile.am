docname = dia

SUBDIRS = en fr eu pl de

EXTRA_DIST = \
	custom-shapes \
	diagram.dtd \
	docbook.xsl \
	shape.dtd \
	sheet.dtd \
	html/css/dia.css html/images/callouts/1.png html/images/callouts/10.png html/images/callouts/11.png html/images/callouts/12.png html/images/callouts/13.png html/images/callouts/14.png html/images/callouts/15.png html/images/callouts/2.png html/images/callouts/3.png html/images/callouts/4.png html/images/callouts/5.png html/images/callouts/6.png html/images/callouts/7.png html/images/callouts/8.png html/images/callouts/9.png html/images/blank.png html/images/caution.png html/images/caution.svg html/images/draft.png html/images/home.png html/images/home.svg html/images/important.png html/images/important.svg html/images/next.png html/images/next.svg html/images/note.png html/images/note.svg html/images/prev.png html/images/prev.svg html/images/tip.png html/images/tip.svg html/images/toc-blank.png html/images/toc-minus.png html/images/toc-plus.png html/images/up.png html/images/up.svg html/images/warning.png html/images/warning.svg

if HAVE_GNOME
helpdir = $(datadir)/gnome/help/$(docname)
else
helpdir = $(pkgdatadir)/help
endif

sysdoc = $(DESTDIR)$(docdir)



# a simple rule to symlink en to C
install-data-local: $(distfiles)
	-(cd $(DESTDIR)$(helpdir) && rm -f C && ln -s en C)
	$(mkinstalldirs) $(sysdoc)
	for i in $^; do \
	  if test -f "$$i"; then  \
	    echo "installing $$i" ;\
	    $(INSTALL_DATA) $$i $(sysdoc)/$$(basename $$i); \
	  fi ; \
	done

uninstall-local:
	rm -f $(DESTDIR)$(helpdir)/C
	-rmdir $(DESTDIR)$(helpdir)
	-rmdir $(sysdoc)/*
	-rmdir $(sysdoc)
	
