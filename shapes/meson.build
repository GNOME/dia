shapesdir = pkgdatadir / 'shapes'

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
subdir('Flowchart')
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
subdir('EDPC')

foreach shape_info : shapes
    install_data(
        shape_info.get('sources'),
        install_dir: shapesdir / shape_info.get('name')
    )
endforeach
