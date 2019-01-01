sheetsdir = join_paths(pkgdatadir, 'sheets')

sheet_icons_sources = [
    'UML/umlclass_template.png',
    'UML/aggregation.png',
    'UML/receptacle.png',
    'UML/eventsource.png',
    'UML/eventsink.png',
    'ER/weakentity.png',
    'GRAFCET/etapei.png',
    'GRAFCET/etapems.png',
    'GRAFCET/etapeme.png',
    'GRAFCET/etapemc.png',
    'GRAFCET/etapesp.png',
    'GRAFCET/vergent_and.png',
    'Jackson/designed_domain.png',
    'Jackson/machine_domain.png',
    'Jackson/req_phen.png',
    'Istar/agent.png',
    'Istar/decomposition.png',
    'Istar/dependency.png',
    'Istar/goal.png',
    'Istar/means_ends.png',
    'Istar/neg_contrib.png',
    'Istar/pos_contrib.png',
    'Istar/position.png',
    'Istar/role.png',
    'Istar/task.png',
    'KAOS/and-complete-ref.png',
    'KAOS/and-ref.png',
    'KAOS/assumption.png',
    'KAOS/capable-of.png',
    'KAOS/conflicts.png',
    'KAOS/controls.png',
    'KAOS/input.png',
    'KAOS/monitors.png',
    'KAOS/obstacle.png',
    'KAOS/obstructs.png',
    'KAOS/op-ref.png',
    'KAOS/or-complete-ref.png',
    'KAOS/or-ref.png',
    'KAOS/output.png',
    'KAOS/performs.png',
    'KAOS/requirement.png',
    'KAOS/responsibility.png',
    'KAOS/softgoal.png'
]

foreach icon : sheet_icons_sources
    rel_path = icon.split('/')
    install_data(icon, install_dir : join_paths(sheetsdir, rel_path[0]))
endforeach

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

# TODO: this needs to depend on whether we have valid instalation of sozi.
if false
    sheet_files += ['Sozi.sheet']
endif

sheet_files_targets = []
# TODO: use gettext:
# https://github.com/mesonbuild/meson/issues/1565
# https://github.com/systemd/systemd/commit/c81217920effddc93fb780cf8f9eb699d6fe1319
# https://wiki.gnome.org/MigratingFromIntltoolToGettext
# https://www.gnu.org/software/gettext/manual/html_node/Preparing-ITS-Rules.html
# https://www.w3.org/TR/its20/ (2013) standard!
foreach sheet : sheet_files
    sheet_files_targets += custom_target(
        sheet,
        input : sheet + '.in',
        output : sheet,
        command : intltool_command,
        install : true,
        install_dir : sheetsdir
    )
endforeach

checktrans_py = find_program('checktrans.py')

custom_target(
    'sheet-translation-report',
    build_by_default :  false,  # Currently broken anyway.
    input : sheet_files_targets,
    output : 'sheet-translation-report',
    command : [checktrans_py, '@INPUT@'],
    capture : true
)
