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
subdir('Sozi')
subdir('standard')
subdir('UML')

foreach o : dia_object_desc
    library(o[0],
        o[1] + [config_h],
        dependencies: [libc_dep, libgtk_dep, libxml_dep, libm_dep] + o[2],
        link_with: libdia,
        include_directories: [configuration_inc, libdia_inc],
        install: true,
        install_dir: dialibdir
    )
endforeach
