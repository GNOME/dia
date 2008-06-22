docname = dia
lang = en
figdir = graphics
omffile = dia-C.omf
entities =
version = 0.5
progname = dia
progfiles_extension = dia
PAPERSIZE = Legal
## document encoding
DOCUMENT_ENCODING = ISO-8859-1
## postscript output resolution (dpi)
RESOLUTION = 600
## dblatex tex document charset
ENCODING = utf8
UNICODE = 1
## TeX papersize/sides
LATEX_CLASS_OPTIONS = -P 'latex.class.options=oneside'


include $(top_srcdir)/xmldocs.make

dist-hook: app-dist-hook

nodist_include_EXTRAS = dia.1



xml_files += \
	authors.xml \
	dia.xml \
	dia-cmdline.xml \
	entities.xml \
	intro.xml \
	license.xml \
	usage-canvas.xml \
	usage-customization.xml \
	usage-layers.xml \
	usage-loadsave.xml \
	usage-objects-basic.xml \
	usage-objects-selecting.xml \
	usage-objects-special.xml \
	usage-objects.xml \
	usage-quickstart.xml \
	custom-shapes.xml \
	dia-1.xml


EXTRA_DIST += \
	dia.dbk \
	dia-1.xml

##	graphics/line_props.png \
##	graphics/home_network.png \
##	graphics/line_icon.png \
##	graphics/greendots.png

pngfigures = \
	graphics/greendots.png \
	graphics/line_icon.png \
	graphics/color_selector.png \
	graphics/home_network.png \
	graphics/line_props.png \
	graphics/preferences_fig.png


epsfigures = \
	ps/graphics/color_selector.eps \
	ps/graphics/preferences_fig.eps

xmlsources = \
	dia-1.xml \
	dia-cmdline.xml \
	authors.xml \
	usage-customization.xml \
	usage-layers.xml \
	custom-shapes.xml \
	dia.xml \
	usage-loadsave.xml \
	entities.xml \
	usage-objects-basic.xml \
	usage-objects-selecting.xml \
	usage-objects-special.xml \
	usage-objects.xml \
	usage-quickstart.xml \
	intro.xml \
	license.xml \
	usage-canvas.xml


htmlstyle = \
	html.xsl


metadata = \
	dia-C.omf \
	topic.dat

examples = \
	$(progname).$(progfiles_extension) \
	../shape.dtd \
	../diagram.dtd \
	../sheet.dtd


DISTCLEANFILES = 

if HAVE_DB2MAN
## Anyone out there who understands automake?
DISTCLEANFILES += dia.1
## If we want misconfigured XML catalogs to not try net access, use
## --nonet.  But I don't think that's going to solve more problems
## than it causes.
dia.1: dia.dbk
	xsltproc --novalid $(DB2MAN) $< >dia.1

man_MANS = dia.1
endif

