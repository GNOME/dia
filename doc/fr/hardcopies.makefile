#
# build and install html, ps and pdf documentation
#
srcdir = .
figdir = graphics
INSTALL_DATA = /usr/bin/install
sysdoc = $(datadir)/doc/$(docname)
papersize = A4
encoding = "utf-8"
eps_figures = color_selector.eps \
			preferences-fig.eps

ifdef WITH_HTMLDOC
htmldoc = dia.html
html_install = install-html
html_uninstall = uninstall-html
html_clean = clean-html
else
htmldoc =
html_install =
html_uninstall =
html_clean =
endif

ifdef WITH_PDFDOC
pdfdoc = dia.pdf
pdf_install = install-pdf
pdf_uninstall = uninstall-pdf
pdf_clean = clean-pdf
else
pdfdoc =
pdf_install =
pdf_uninstall =
pdf_clean =
endif

ifdef WITH_PSDOC
psdoc = dia.ps
ps_install = install-ps
ps_uninstall = uninstall-ps
ps_clean = clean-ps
else
psdoc =
ps_install =
ps_uninstall =
ps_clean =
endif

all: mk-chapter-cmdline

dia.html: dia.xml
	install -d $(srcdir)/html
	cd $(srcdir)/html && xsltproc ../html.xsl ../dia.xml


dia.pdf: dia.xml
	export SP_ENCODING=$(encoding) ; \
	jw -b pdf -V paper-type=$(papersize) -V paper-size=$(papersize) -o pdf/  $<

dia.ps: dia.xml
	mkdir -p $(srcdir)/ps/$(figdir)/
	if test "$(figdir)"; then \
	  for file in $(figdir)/*.png ; do \
	    destfile=$(srcdir)/ps/$(figdir)/`basename $$file .png`.eps ; \
	    if test ! -f $$destfile ; then \
	        convert $(srcdir)/$$file $$destfile; \
	    fi ; \
	  done ; \
	fi
	sed -e 's/<!ENTITY pngfile "png">/<!ENTITY pngfile "eps">/' $< >doc-ps.xml
	export SP_ENCODING=$(encoding) ; \
    jw -b ps -V paper-type=$(papersize) -V paper-size=$(papersize) -o ps/ doc-ps.xml
	rm doc-ps.xml
	mv ps/doc-ps.ps ps/`basename $< .xml`.ps
 
dia.tex: dia.xml
	mkdir -p $(srcdir)/ps/$(figdir)/
	if test "$(figdir)"; then \
	  for file in $(figdir)/*.png ; do \
	    destfile=$(srcdir)/ps/$(figdir)/`basename $$file .png`.eps ; \
	    if test ! -f $$destfile ; then \
	        convert $(srcdir)/$$file $$destfile; \
	    fi ; \
	  done ; \
	fi
	sed -e 's/<!ENTITY pngfile "png">/<!ENTITY pngfile "eps">/' $< >doc-ps.xml
	export SP_ENCODING=$(encoding) ; \
    jw -b tex -V paper-type=$(papersize) -V paper-size=$(papersize) -o ps/ doc-ps.xml
	rm doc-ps.xml
	mv ps/doc-ps.tex ps/`basename $< .xml`.tex
    
mk-chapter-cmdline: dia.dbk
	sed -f dia-dbk-to-chapter.sed $< > dia-cmdline.xml
	 
pdf_docs = \
	dia.pdf
	 
ps_docs = \
	dia.ps

dia_examples = \
	dia.dia


install-html:
	@list = `ls $(srcdir)/html`
	$(mkinstalldirs) $(sysdoc)
	$(mkinstalldirs) $(sysdoc)/$(lang)
	$(mkinstalldirs) $(sysdoc)/$(lang)/$(figdir)
	for i in $$list; do \
	  file=$(srcdir)/html/$$i; \
	  dest=`echo $$file | sed -e 's,^.*/,,'`; \
	  echo " $(INSTALL_DATA) $$file $(sysdoc)/$(lang)/$$dest"; \
	  $(INSTALL_DATA) $$file $(sysdoc)/$(lang)/$$dest; \
	done
	@list=`ls $(srcdir)/$(figdir)`
	for i in $$list; do \
	  file=$(srcdir)/$(figdir)/$$i; \
	  dest=`echo $$file | sed -e 's,^.*/,,'`; \
	  echo " $(INSTALL_DATA) $$file $(sysdoc)/$(lang)/$$dest"; \
	  $(INSTALL_DATA) $$file $(sysdoc)/$(lang)/$$dest; \
	done
	

uninstall-html:
	@list='$(html_pages)'; \
	for i in $$list ; do \
	  file=`echo $$i | sed -e 's,^\.*/,,'`; \
	  if test -f $(helpdir)/$(lang)/$$file ; then \
	    echo " rm -f $(helpdir)/$(lang)/$$file"; \
	    rm -f $(helpdir)/$(lang)/$$file; \
	  fi; \
	done
	@list=`ls $(srcdir)/$(figdir)`; \
	for i in $$list ; do \
	  file=`echo $$i | sed -e 's,^\.*/,,'`; \
	  if test -f $(helpdir)/$(lang)/$$file ; then \
	    echo " rm -f $(helpdir)/$(lang)/$(figdir)/$$file"; \
	    rm -f $(helpdir)/$(lang)/$(figdir)/$$lfile; \
	  fi; \
	done

clean-html:
	@list='$(html_pages)'; \
	for i in $$list ; do \
	  if test -f $(srcdir)/$$i; then \
	    file=$(srcdir)/$$i; \
	    else file=$$i;\
	  fi; \
	  if test -f $$file ; then \
	    echo " rm -f $$file"; \
	    rm -f $$file; \
	  fi; \
	done


install-pdf:
	$(mkinstalldirs) $(sysdoc)/$(lang)
	@list='$(pdf_docs)'; \
	for i in $$list; do \
	  if test -f $(srcdir)/pdf/$$i; then  \
	    file=$(srcdir)/pdf/$$i; \
	  else \
	    file=$$i; \
	  fi; \
	  if test ! -f $$file ; then continue ; fi ; \
	  dest=`echo $$file | sed -e 's,^\.*/,,'`; \
	  echo " $(INSTALL_DATA) $$file $(sysdoc)/$(lang)/$$dest"; \
	  $(INSTALL_DATA) $$file $(sysdoc)/$(lang)/$$dest; \
	done

uninstall-pdf:
	@list='$(pdf_docs)'; \
	for i in $$list ; do \
	  pdffile=`echo $$i | sed -e 's,^\.*/,,'`; \
	  if test -f $(sysdoc)/$(lang)/$$pdffile ; then \
	    echo " rm -f $(sysdoc)/$(lang)/$$pdffile"; \
	    rm -f $(sysdoc)/$(lang)/$$pdffile; \
	  fi; \
	done
	rmdir $(sysdoc)/$(lang)
	rmdir $(sysdoc)

clean-pdf: 
	rm $(srcdir)/pdf/*
	rmdir $(srcdir)/pdf/


install-ps:
	$(mkinstalldirs) $(sysdoc)/$(lang)
	@list='$(ps_docs)'; \
	for i in $$list; do \
	  if test -f $(srcdir)/ps/$$i; then  \
	    file=$(srcdir)/ps/$$i; \
	  else \
	    file=$$i; \
	  fi; \
	  if test ! -f $$file ; then continue ; fi ; \
	  dest=`echo $$file | sed -e 's,^\.*/,,'`; \
	  echo " $(INSTALL_DATA) $$file $(sysdoc)/$(lang)/$$dest"; \
	  $(INSTALL_DATA) $$file $(sysdoc)/$(lang)/$$dest; \
	done

uninstall-ps:
	@list='$(ps_docs)'; \
	for i in $$list ; do \
	  psfile=`echo $$i | sed -e 's,^\.*/,,'`; \
	  if test -f $(sysdoc)/$(lang)/$$psfile ; then \
	    echo " rm -f $(sysdoc)/$(lang)/$$psfile"; \
	    rm -f $(sysdoc)/$(lang)/$$psfile; \
	  fi; \
	done

clean-ps:
	@list='$(ps_docs)'; \
    for i in $$list ; do \
	    rm $(srcdir)/ps/$$i ; \
    done
	@list='$(eps_figures)' ; \
    for file in `ls ps/graphics/` ; do \
        keep=0 ; \
        for to_keep in @list ; do \
            if test "$$file" = "$$to_keep" ; then \
            keep=1 ; \
        done ; \
        if test $$keep = 0 ; then \
            echo "rm -f ps/graphics/$$file" ; \
            rm -f ps/graphics/$$file \
        fi \
    done   

install-examples:
	$(mkinstalldirs) $(sysdoc)/$(lang)
	@list='$(dia_examples)'; \
	for i in $$list; do \
	  if test -f $(srcdir)/$$i; then  \
	    file=$(srcdir)/$$i; \
	  else \
	    file=$$i; \
	  fi; \
	  if test ! -f $$file ; then continue ; fi ; \
	  dest=`echo $$file | sed -e 's,^\.*/,,'`; \
	  echo " $(INSTALL_DATA) $$file $(sysdoc)/$(lang)/$$dest"; \
	  $(INSTALL_DATA) $$file $(sysdoc)/$(lang)/$$dest; \
	done


uninstall-examples:
	@list='$(dia_examples)'; \
	for i in $$list ; do \
	  file=`echo $$i | sed -e 's,^\.*/,,'`; \
	  if test -f $(sysdoc)/$(lang)/$$file ; then \
	    echo " rm -f $(sysdoc)/$(lang)/$$file"; \
	    rm -f $(sysdoc)/$(lang)/$$file; \
	  fi; \
	done


cmdline-clean:
	@list='dia-cmdline.xml'; \
	for file in $$list ; do \
	  if test -f $(srcdir)/$$file ; then \
	    echo " rm -f $(srcdir)/$$file"; \
	    rm -f $(srcdir)/$$file; \
	  fi; \
	done

