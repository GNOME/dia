SUBDIRS = UML ER GRAFCET Jackson Istar KAOS

# To create a .sheet.in file from a .sheet file, run
# sed 's!<description!<_description!;s!</description!</_description!;s!<name!<_name!;s!</name!</_name!;' 

sheet_in_files = \
  UML.sheet.in ER.sheet.in sybase.sheet.in FS.sheet.in network.sheet.in \
  Flowchart.sheet.in Circuit.sheet.in Contact.sheet.in GRAFCET.sheet.in \
  chronogram.sheet.in SADT.sheet.in Pneumatic.sheet.in Electric.sheet.in \
  civil.sheet.in jigsaw.sheet.in MSE.sheet.in SDL.sheet.in Logic.sheet.in \
  Misc.sheet.in Assorted.sheet.in ciscocomputer.sheet.in \
  ciscohub.sheet.in ciscomisc.sheet.in cisconetwork.sheet.in \
  ciscotelephony.sheet.in Cybernetics.sheet.in IsometricMap.sheet.in \
  Istar.sheet.in Jackson.sheet.in KAOS.sheet.in ChemEng.sheet.in \
  AADL.sheet.in Gane_and_Sarson.sheet.in BPMN.sheet.in \
  Lights.sheet.in Database.sheet.in Sozi.sheet.in Shape_Design.sheet.in

SHEETS = $(sheet_in_files:.sheet.in=.sheet)

sheetdir = $(pkgdatadir)/sheets
sheet_DATA = $(SHEETS)

%.sheet: %.sheet.in $(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*.po) 
	$(INTLTOOL_MERGE) $(top_srcdir)/po $< $@ -x -u \
	 	-c $(top_srcdir)/po/.intltool-merge-cache

EXTRA_DIST = $(sheet_in_files) \
	checktrans checktrans.py

CLEANFILES = sheet-translation-report $(SHEETS) 

sheet-translation-report: $(SHEETS) checktrans checktrans.py
	-(cd $(srcdir) && ./checktrans $(SHEETS))

all-local: sheet-translation-report

