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

# The following is necessary, because most .po files are not UTF-8, and we 
# don't yet want to force people to use UTF-8 everywhere.
stamp-po-files: ../po/*.po
	echo $(sheet_in_files)
	echo $(sheet_DATA)
	install -d temp-po
	cp ../po/*.po temp-po
	(cd temp-po ; \
         for po in *.po ; \
	 do \
	   xml-i18n-unicodify `basename $$po .po` ; \
	   mv $$po-1 $$po ; \
	 done )
	touch stamp-po-files


%.sheet : %.sheet.in $(top_builddir)/xml-i18n-merge stamp-po-files
	$(top_builddir)/xml-i18n-merge -x temp-po $< $*.sheet

EXTRA_DIST = sheet-translation-report $(SHEETS) $(sheet_in_files) \
	checktrans checktrans.py

CLEANFILES = sheet-translation-report stamp-po-files $(SHEETS) temp-po/*

sheet-translation-report: $(SHEETS) checktrans checktrans.py
	-(cd $(srcdir) && ./checktrans $(SHEETS))

all-local: sheet-translation-report