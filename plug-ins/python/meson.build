# TODO: should we use pymod.find_installation() ?
#       this seems to work fine.
# TODO: is just the only dependency we need?
# Ensure that this does not require pygtk-2.0 as per
# Makefile.am.
py_dep = dependency('python2', version: '>= 2.3', required: false)

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
    'gtkcons.py',
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
        sources,
        dependencies: [libxml_dep, libgtk_dep, py_dep],
        include_directories: [libdia_inc, configuration_inc, diaapp_inc],
        link_with: [libdia],
        install: true,
        install_dir: dialibdir
    )

    install_data(python_scripts, install_dir: join_paths(pkgdatadir, 'python'))
    install_data('python-startup.py', install_dir: pkgdatadir)
endif
