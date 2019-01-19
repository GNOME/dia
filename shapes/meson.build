shapesdir = join_paths(pkgdatadir, 'shapes')

shapes = []

subdir('Assorted')
subdir('BPMN')
subdir('ChemEng')
subdir('Circuit')
subdir('Cisco')
subdir('Civil')
subdir('Contact')
subdir('Cybernetics')
subdir('Electric')
subdir('flowchart')
subdir('Gane_and_Sarson')
subdir('jigsaw')
subdir('Lights')
subdir('Logic')
subdir('Map')
subdir('Misc')
subdir('MSE')
subdir('network')
subdir('Pneumatic')
subdir('SDL')
subdir('Shape_Design')
subdir('sybase')

foreach shape_info : shapes
    install_data(
        shape_info[0],
        install_dir: join_paths(shapesdir, shape_info[1])
    )
endforeach
