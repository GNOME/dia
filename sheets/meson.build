sheetsdir = join_paths(pkgdatadir, 'sheets')

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
]

sheet_files_targets = []

# TODO: use gettext: https://github.com/mesonbuild/meson/issues/1565
# https://github.com/systemd/systemd/commit/c81217920effddc93fb780cf8f9eb699d6fe1319
# https://wiki.gnome.org/MigratingFromIntltoolToGettext
# https://www.gnu.org/software/gettext/manual/html_node/Preparing-ITS-Rules.html
# https://www.w3.org/TR/its20/ (2013) standard!
foreach sheet : sheet_files
    sheet_files_targets += custom_target(
        sheet,
        input: sheet + '.in',
        output: sheet,
        command: intltool_command,
        install: true,
        install_dir: sheetsdir
    )
endforeach

checktrans_py = find_program('checktrans.py')

custom_target(
    'sheet-translation-report',
    build_by_default:  false,  # TODO: tracked in issue #24.
    input: sheet_files_targets,
    output: 'sheet-translation-report',
    command: [checktrans_py, '@INPUT@'],
    capture: true
)
