#!/usr/bin/env python3
import os
import subprocess
import sys

# Inspired from https://gitlab.gnome.org/GNOME/evince/blob/d69158ecf0e2a2f1562b06c265fc86f87fe7dd6f/meson_post_install.py

if not os.environ.get('MESON_INSTALL_DESTDIR_PREFIX'):
    datadir = sys.argv[1]
    icondir = os.path.join(datadir, 'icons', 'hicolor')

    print('Updating icon cache...')
    subprocess.call(['gtk-update-icon-cache', '-f', '-t', icondir])

    print('Updating desktop database...')
    subprocess.call(['update-desktop-database', os.path.join(datadir, 'applications')])
