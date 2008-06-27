
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

sysdoc = $(DESTDIR)$(datadir)/doc/$(docname)/$(lang)

if WITH_HTMLDOC
htmldoc = $(progname)_html
html_install = install-html
html_uninstall = uninstall-html
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

all: omf $(htmldoc) $(pdfdoc) $(psdoc)

install-data-local: install-data-xml \
	$(html_install) $(pdf_install) $(ps_install) install-examples

uninstall-local: uninstall-local-xml  \
	uninstall-html uninstall-pdf uninstall-ps \
	uninstall-ps uninstall-pdf uninstall-examples

clean-local: clean-local-xml \
	clean-html clean-ps clean-pdf

$(progname)_html: $(progname).xml $(xmlsources) $(htmlstyle) $(pngfigures)
	$(mkinstalldirs) $(srcdir)/$(progname)_html
	$(mkinstalldirs) $(srcdir)/$(progname)_html/$(figdir)
	$(mkinstalldirs) $(srcdir)/$(progname)_html/images
	$(mkinstalldirs) $(srcdir)/$(progname)_html/images/callouts
	$(mkinstalldirs) $(srcdir)/$(progname)_html/css
	-cp ../html/images/*.png $(srcdir)/$(progname)_html/images
	-cp ../html/images/callouts/*.png \
	  $(srcdir)/$(progname)_html/images/callouts
	-cp ../html/css/*.css $(srcdir)/$(progname)_html/css
	-cp $(srcdir)/$(figdir)/*.png $(srcdir)/$(progname)_html/$(figdir)
	cd $(srcdir)/$(progname)_html \
	  && xsltproc --stringparam graphic.default.extension png \
	    ../$(htmlstyle) ../$<
	touch $(progname)_html

if WITH_PDFDOC
$(progname).pdf: $(progname).xml $(xmlsources) $(pngfigures)
	-$(DBLATEX) -t pdf -T  native \
		-P 'latex.unicode.use=$(UNICODE)' \
		-P latex.encoding='$(ENCODING)' \
		$(LATEX_CLASS_OPTIONS) \
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
$(progname).ps: $(progname).xml $(xmlsources) .epsfigures
	-$(DBLATEX) -t ps -T native \
	        $(stylesheet) \
		-P 'latex.unicode.use=$(UNICODE)' \
		-P 'latex.encoding=$(ENCODING)' \
		$(LATEX_CLASS_OPTIONS) \
		$(srcdir)/$<
endif

if WITH_JW
.jw: $(progname).xml $(xmlsources) .epsfigures
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

install-html: $(progname)_html
	$(mkinstalldirs) $(sysdoc)/html
	cp -r $(srcdir)/$(progname)_html/* $(sysdoc)/html

uninstall-html:
	-rm -f $(sysdoc)/html/*.html
	-rm -f $(sysdoc)/html/$(figdir)/*
	-rm -f $(sysdoc)/html/images/callouts/*
	-rm -f $(sysdoc)/html/images/*
	-rm -f $(sysdoc)/html/css/*

clean-html:
	-rm -rf $(srcdir)/$(progname)_html


install-pdf: $(progname).pdf
	$(mkinstalldirs) $(sysdoc)
	-$(INSTALL_DATA) $< $(sysdoc)/$<

uninstall-pdf:
	-rm -f $(sysdoc)/$(progname).pdf

clean-pdf:
	rm -f $(progname).pdf
	-rm -rf $(srcdir)/jw
	rm -f .jw

install-ps: $(progname).ps
	$(mkinstalldirs) $(sysdoc)
	-$(INSTALL_DATA) $< $(sysdoc)/$<

uninstall-ps:
	-rm -f $(sysdoc)/$(progname).ps

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
