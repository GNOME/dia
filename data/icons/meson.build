icon_files = {
    '16x16': 'dia.png',
    '22x22': 'dia.png',
    '24x24': 'dia.png',
    '32x32': 'dia.png',
    '48x48': 'dia.png',
    'scalable': 'dia.svg',
}

icondir = join_paths(datadir, 'icons', 'hicolor')

# Remember to call gtk-update-icon-cache when installing / uninstalling.
foreach size, file : icon_files
    install_data(
        join_paths(size, 'apps', file),
        install_dir: join_paths(icondir, size, 'apps')
    )
endforeach
