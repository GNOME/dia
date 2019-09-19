icon_files = {
    '16x16': 'org.gnome.Dia.png',
    '22x22': 'org.gnome.Dia.png',
    '24x24': 'org.gnome.Dia.png',
    '32x32': 'org.gnome.Dia.png',
    '48x48': 'org.gnome.Dia.png',
    'scalable': 'org.gnome.Dia.svg',
}

icondir = join_paths(datadir, 'icons', 'hicolor')

# Remember to call gtk-update-icon-cache when installing / uninstalling.
foreach size, file : icon_files
    install_data(
        join_paths(size, 'apps', file),
        install_dir: join_paths(icondir, size, 'apps')
    )
endforeach
