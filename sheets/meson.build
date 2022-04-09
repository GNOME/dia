sheetsdir = pkgdatadir / 'sheets'

sheet_files = [
    'UML.sheet',
    'ER.sheet',
    'sybase.sheet',
    'FS.sheet',
    'network.sheet',
    'Flowchart.sheet',
    'Circuit.sheet',
    'Contact.sheet',
    'GRAFCET.sheet',
    'chronogram.sheet',
    'SADT.sheet',
    'Pneumatic.sheet',
    'Electric.sheet',
    'civil.sheet',
    'jigsaw.sheet',
    'MSE.sheet',
    'SDL.sheet',
    'Logic.sheet',
    'Misc.sheet',
    'Assorted.sheet',
    'ciscocomputer.sheet',
    'ciscohub.sheet',
    'ciscomisc.sheet',
    'cisconetwork.sheet',
    'ciscotelephony.sheet',
    'Cybernetics.sheet',
    'IsometricMap.sheet',
    'Istar.sheet',
    'Jackson.sheet',
    'KAOS.sheet',
    'ChemEng.sheet',
    'AADL.sheet',
    'Gane_and_Sarson.sheet',
    'BPMN.sheet',
    'Lights.sheet',
    'Database.sheet',
    'Shape_Design.sheet',
    'EDPC.sheet',
]

sheet_files_targets = []

foreach sheet : sheet_files
    sheet_files_targets += i18n.merge_file(
        input: sheet,
        output: sheet,
        po_dir: po_dir,
        data_dirs: po_dir,
        install: true,
        install_dir: sheetsdir
    )
endforeach
