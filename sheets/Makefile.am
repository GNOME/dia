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

sheetdir = $(pkgdatadir)/sheets
sheet_in_files = $(SHEETS:.sheet=.sheet.in)
sheet_DATA = $(sheet_in_files:.sheet.in=.sheet)

### <RUBBISH>
%.sheet : %.sheet.in $(top_builddir)/xml-i18n-merge ../po/*.po
	$(top_builddir)/xml-i18n-merge -x -u ../po $< $*.sheet
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