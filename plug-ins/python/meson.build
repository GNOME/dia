py3_dep = dependency('python3-embed', version: '>= 3.8')

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

if py3_dep.found()
    # Same as layout plugin.
    shared_module(
        'python_plugin',
        sources,
        dependencies: [libc_dep, libxml_dep, libgtk_dep, py3_dep, libdia_dep] + [config_dep],
        link_with: [diaapp], # Naughty
        install: true,
        install_dir: dialibdir,
        include_directories: diaapp_inc,
        name_suffix: g_module_suffix,
        # Python headers trigger this warning. Let's not obscure the others.
        c_args: [
          '-Wno-declaration-after-statement',
        ]
    )

    install_data(python_scripts, install_dir: pkgdatadir / 'python')
    install_data('python-startup.py', install_dir: pkgdatadir)
endif
