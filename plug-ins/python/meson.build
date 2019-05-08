pymod = import('python')

py_inst = pymod.find_installation('python2', required: false)
py_dep = py_inst.dependency(version: '>= 2.3')

sources = files(
    'pydia-color.c',
    'pydia-cpoint.c',
    'pydia-diagram.c',
    'pydia-diagramdata.c',
    'pydia-display.c',
    'pydia-error.c',
    'pydia-export.c',
    'pydia-font.c',
    'pydia-geometry.c',
    'pydia-handle.c',
    'pydia-image.c',
    'pydia-layer.c',
    'pydia-menuitem.c',
    'pydia-object.c',
    'pydia-paperinfo.c',
    'pydia-properties.c',
    'pydia-property.c',
    'pydia-render.c',
    'pydia-sheet.c',
    'pydia-text.c',
    'diamodule.c',
    'python.c',
)

python_scripts = [
    'aobjects.py',
    'allprops.py',
    'codegen.py',
    'diasvg.py',
    'diasvg_import.py',
    'dot.py',
    'doxrev.py',
    'gtkcons.py',  # Will only run if pygtk is present.
    'imgmap.py',
    'otypes.py',
    'pydiadoc.py',
    'scascale.py',
    'select_by.py',
    'select_empty.py',
]

if py_dep.found()
    # Same as layout plugin.
    shared_module(
        'python_plugin',
        sources + [config_h],
        dependencies: [libc_dep, libxml_dep, libgtk_dep, py_dep, libdia_dep],
        include_directories: [configuration_inc, diaapp_inc],
        link_with: [diaapp],
        install: true,
        install_dir: dialibdir
    )

    install_data(python_scripts, install_dir: join_paths(pkgdatadir, 'python'))
    install_data('python-startup.py', install_dir: pkgdatadir)
endif
