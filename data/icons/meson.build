icons_sources = [
    '16x16/apps/dia.png',
    '22x22/apps/dia.png',
    '24x24/apps/dia.png',
    '32x32/apps/dia.png',
    '48x48/apps/dia.png',
    'scalable/apps/dia.svg'
]

icondir = join_paths(datadir, 'icons', 'hicolor')

# Remember to call gtk-update-icon-cache when installing / uninstalling.

# It would be nice if install_subdir allowed selecting files.
foreach icon : icons_sources
    rel_path = icon.split('/')
    install_data(icon, install_dir : join_paths(icondir,
                                                rel_path[0],
                                                rel_path[1]))
endforeach
