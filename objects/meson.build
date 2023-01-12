dia_object_desc = []

subdir('AADL')
subdir('chronogram')
subdir('custom')
subdir('custom_lines')
subdir('Database')
subdir('ER')
subdir('flowchart')
subdir('FS')
subdir('GRAFCET')
subdir('Istar')
subdir('Jackson')
subdir('KAOS')
subdir('Misc')
subdir('network')
subdir('SADT')
subdir('standard')
subdir('UML')

foreach o : dia_object_desc
    library(o.get('name'),
        o.get('sources'),
        dependencies: [libc_dep, libgtk_dep, libxml_dep, libm_dep, libdia_dep] + [config_dep] + o.get('extra_deps', []),
        include_directories: [o.get('inc', [])],
        install: true,
        install_dir: dialibdir,
        name_suffix: g_module_suffix,
    )
endforeach
