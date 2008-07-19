docname=dia
lang=pl
figdir=graphics
omffile=dia-pl.omf
entities=
progname = dia
progfiles_extension = dia
PAPERSIZE = A4
## postscript ouput resolution (dpi)
RESOLUTION = 600
## document encoding
DOCUMENT_ENCODING = ISO-8859-2
## dblatex tex document charset
ENCODING = utf8
UNICODE = 1
## TeX papersize/sides
LATEX_CLASS_OPTIONS = -P 'latex.class.options=oneside' -P 'latex.babel.use=0'


include $(top_srcdir)/xmldocs.make

dist-hook: app-dist-hook

nodist_include_EXTRAS = dia.1



xml_files += \
	authors.xml \
	dia.xml \
	intro.xml \
	license.xml \
	usage-canvas.xml \
	usage-customization.xml \
	usage-layers.xml \
	usage-loadsave.xml \
	usage-objects-selecting.xml \
	usage-objects-special.xml \
	usage-objects.xml \
	usage-quickstart.xml 

##	usage-objects-basic.xml \
##	dia-cmdline.xml \
##	entities.xml \
##	dia-1.xml


EXTRA_DIST += \
	html.xsl

##	dia.dbk \
##	dia-1.xml
##	dia-dbk-to-chapter.sed
##	graphics/line_props.png \
##	graphics/home_network.png \
##	graphics/line_icon.png \
##	graphics/greendots.png

pngfigures = \
	graphics/greendots.png \
	graphics/home_network.png \
	graphics/line_props.png

##	graphics/line_icon.png \

epsfigures = 

htmlstyle = \
	html.xsl


metadata = \
	dia-pl.omf \
	topic.dat

examples = \
	../shape.dtd \
	../diagram.dtd \
	../sheet.dtd

##	$(progname).$(progfiles_extension) \

DISTCLEANFILES = 

##if HAVE_DB2MAN
### Anyone out there who understands automake?
##DISTCLEANFILES += dia.1
### If we want misconfigured XML catalogs to not try net access, use
### --nonet.  But I don't think that's going to solve more problems
### than it causes.
##dia.1: dia.dbk
##	xsltproc --novalid $(DB2MAN) $< >dia.1
##
##man_MANS = dia.1
##endif
##
##
