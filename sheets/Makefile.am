SUBDIRS = UML ER GRAFCET

SHEETS = \
	UML.sheet  \
	ER.sheet \
	sybase.sheet \
	FS.sheet \
	network.sheet \
	Flowchart.sheet \
	Circuit.sheet \
	Contact.sheet \
	GRAFCET.sheet \
	chronogram.sheet \
	SADT.sheet \
	Pneumatic.sheet \
	Electric.sheet \
	civil.sheet \
	jigsaw.sheet \
	MSE.sheet \
	SDL.sheet \
        Logic.sheet \
	EML.sheet

sheet_in_files = \
  UML.sheet.in ER.sheet.in sybase.sheet.in FS.sheet.in network.sheet.in \
  Flowchart.sheet.in Circuit.sheet.in Contact.sheet.in GRAFCET.sheet.in \
  chronogram.sheet.in SADT.sheet.in Pneumatic.sheet.in Electric.sheet.in \
  civil.sheet.in jigsaw.sheet.in MSE.sheet.in SDL.sheet.in Logic.sheet.in \
  EML.sheet.in

sheetdir = $(pkgdatadir)/sheets
sheet_DATA = $(SHEETS)

### <RUBBISH>
$(SHEETS): build-sheets.stamp

build-sheets.stamp: $(sheet_in_files)
	for sheet in $(SHEETS); do \
	  $(top_builddir)/intltool-merge -x -u $(top_srcdir)/po $(srcdir)/$$sheet.in $$sheet; \
	done
	touch build-sheets.stamp

distclean-hook:
	rm -f *.sheet build-sheets.stamp

### </RUBBISH>

# The above will go as soon as we get a fixed (post 0.9) xml-i18n-tools 
# released, and correctly distributed ("testing" + "rawhide" + two weeks 
# minimum). When that happens, we just uncomment the following and remove 
# this rule (this used to be much, much worse).
#@XML_I18N_MERGE_SHEET_RULE

EXTRA_DIST = sheet-translation-report $(sheet_in_files) \
	checktrans checktrans.py

CLEANFILES = sheet-translation-report $(SHEETS) 

sheet-translation-report: $(SHEETS) checktrans checktrans.py
	-(cd $(srcdir) && ./checktrans $(SHEETS))

all-local: sheet-translation-report

