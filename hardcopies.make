
##
## hardcopies.make
##
## HTML, PDF and Postcript documentation generation
## DO NOT MODIFY THIS FILE 
##
## COPY doc/en/Makefile.am in your doc/[language] directory,
## then edit variables as needed
##
##

sysdoc = $(DESTDIR)$(docdir)/$(lang)
sysdoc_html = $(DESTDIR)$(htmldir)/html/$(lang)
sysdoc_pdf = $(DESTDIR)$(pdfdir)/$(lang)
sysdoc_ps = $(DESTDIR)$(psdir)/$(lang)

if WITH_HTMLDOC
htmldoc = $(progname)_html
html_install_data = install-data-html
html_install = install-html
html_install_ng = install-html-nognome
html_uninstall = uninstall-html
html_uninstall_ng = uninstall-html-nognome
html_clean = clean-html
endif

if WITH_JW
pdfdoc = $(progname).pdf
pdf_install = install-pdf
psdoc = $(progname).ps
ps_install = install-ps
endif

if WITH_PDFDOC
pdfdoc = $(progname).pdf
pdf_install = install-pdf
endif

if WITH_PSDOC
psdoc = $(progname).ps
ps_install = install-ps
endif

if HAVE_GNOME
all: omf $(htmldoc) $(pdfdoc) $(psdoc)

install-data-local: install-data-xml \
	$(html_install) $(pdf_install) $(ps_install) install-examples

uninstall-local: uninstall-local-xml  \
	$(html_install) uninstall-pdf uninstall-ps uninstall-examples

clean-local: clean-local-xml \
	clean-html clean-ps clean-pdf

else
 
if HAVE_XSLTPROC
all: $(htmldoc) $(pdfdoc) $(psdoc)

install-data-local: $(html_install_data) \
	$(html_install_ng) $(pdf_install) $(ps_install) install-examples
 
uninstall-local: uninstall-local-html  \
	uninstall-html-nognome uninstall-pdf uninstall-ps uninstall-examples
else
all: $(htmldoc) $(pdfdoc) $(psdoc)

install-data-local: $(html_install) $(pdf_install) $(ps_install) install-examples
 
uninstall-local: uninstall-html uninstall-pdf uninstall-ps uninstall-examples
endif

clean-local: clean-local-xml \
	clean-html clean-ps clean-pdf
endif

$(progname)_html: $(progname).xml $(xml_files) $(htmlstyle) $(pngfigures)
	$(mkinstalldirs) $(progname)_html
	$(mkinstalldirs) $(progname)_html/$(figdir)
	$(mkinstalldirs) $(progname)_html/images
	$(mkinstalldirs) $(progname)_html/images/callouts
	$(mkinstalldirs) $(progname)_html/css
	-cp $(srcdir)/../html/images/*.png $(progname)_html/images
	-cp $(srcdir)/../html/images/callouts/*.png \
	  $(progname)_html/images/callouts
	-cp $(srcdir)/../html/css/*.css $(progname)_html/css
	-cp $(srcdir)/$(figdir)/*.png $(progname)_html/$(figdir)
	cd $(progname)_html \
	  && xsltproc --stringparam graphic.default.extension png \
	    ../$(srcdir)/$(htmlstyle) ../$<
	touch $(progname)_html

if WITH_PDFDOC
$(progname).pdf: $(progname).xml $(xml_files) $(pngfigures)
	-$(DBLATEX) -t pdf -T  native \
		-P 'latex.unicode.use=$(UNICODE)' \
		-P latex.encoding='$(ENCODING)' \
		$(LATEX_CLASS_OPTIONS) \
		-o $@ \
		$<
endif

.epsfigures: $(epsfigures) $(pngfigures)
	-if test -d "$(srcdir)/$(figdir)"; then \
	  for file in $(pngfigures) ; do \
	    destfile=$(srcdir)/$(figdir)/`basename $$file .png`.eps ; \
	    convert $$file $$destfile; \
	  done ; \
	fi
	-if test -d "$(srcdir)/ps/$(figdir)"; then \
	  for file in $(epsfigures) ; do \
	    cp  $$file $(srcdir)/$(figdir)/ ; \
	  done ; \
	fi
	touch .epsfigures

if WITH_PSDOC
$(progname).ps: $(progname).xml $(xml_files) .epsfigures
	-$(DBLATEX) -t ps -T native \
	        $(stylesheet) \
		-P 'latex.unicode.use=$(UNICODE)' \
		-P 'latex.encoding=$(ENCODING)' \
		$(LATEX_CLASS_OPTIONS) \
		$(srcdir)/$<
endif

if WITH_JW
.jw: $(progname).xml $(xml_files) .epsfigures
	rm -rf jw
	mkdir -p jw
	cp -r graphics jw
	cp *.xml jw
	flip -b -u *.xml
	cd jw && recode -d '$(DOCUMENT_ENCODING)..XML-standalone' *.xml
	touch .jw

$(progname).ps: .jw
	cd jw && $(JW) -b ps -V paper-type=$(PAPERSIZE) $(progname).xml
	cp jw/$(progname).ps .

$(progname).pdf: .jw
	cd jw && $(JW) -b pdf -V paper-type=$(PAPERSIZE) $(progname).xml
	cp jw/$(progname).pdf .
endif

install-html: $(htmldoc)
	$(mkinstalldirs) $(sysdoc_html)
	cp -r $(progname)_html/* $(sysdoc_html)

uninstall-html:
	-rm -f $(sysdoc_html)/*.html
	-rm -f $(sysdoc_html)/$(figdir)/*.png
	-rmdir $(sysdoc_html)/$(figdir)
	-rm -f $(sysdoc_html)/images/callouts/*.png
	-rmdir $(sysdoc_html)//images/callouts
	-rm -f $(sysdoc_html)/images/*.png
	-rmdir $(sysdoc_html)/images/
	-rm -f $(sysdoc_html)/css/*.css
	-rmdir $(sysdoc_html)/css
	-rmdir $(sysdoc_html)

install-html-nognome:
	$(mkinstalldirs) $(DESTDIR)$(htmldir)/html/
	-(cd $(DESTDIR)$(htmldir)/html/ && ln -s $(helpdocdir) $(lang))

uninstall-html-nognome:
	-rm $(DESTDIR)$(htmldir)/html/$(lang)
	-rmdir $(DESTDIR)$(htmldir)/html/

clean-html:
	-rm -rf $(srcdir)/$(progname)_html


install-pdf: $(pdfdoc)
	$(mkinstalldirs) $(sysdoc_pdf)
	-$(INSTALL_DATA) $< $(sysdoc_pdf)/$<

uninstall-pdf:
	-rm -f $(sysdoc_pdf)/$(progname).pdf

clean-pdf:
	rm -f $(progname).pdf
	-rm -rf $(srcdir)/jw
	rm -f .jw

install-ps: $(progname).ps
	$(mkinstalldirs) $(sysdoc_ps)
	-$(INSTALL_DATA) $< $(sysdoc_ps)/$<

uninstall-ps:
	-rm -f $(sysdoc_ps)/$(progname).ps

clean-ps:
	-rm -rf $(srcdir)/tex
	-rm -f $(progname).ps
	-rm -f .epsfigures
	-rm -f $(srcdir)/graphics/*.eps
	-rm -rf $(srcdir)/jw
	rm -f .jw

install-examples: $(examples)
	$(mkinstalldirs) $(sysdoc)/examples
	for i in $^; do \
	  if test -f "$$i"; then  \
	    echo "installing $$i" ;\
	    $(INSTALL_DATA) $$i $(sysdoc)/examples/$$(basename $$i) ; \
	  fi ; \
	done

uninstall-examples: $(examples)
	for i in $^; do \
	  rm -f $(sysdoc)/examples/$$(basename $$i) ;\
	done
	-rmdir $(sysdoc)/examples

