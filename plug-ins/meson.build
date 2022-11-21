# First element is plugin name
# Second is source list
# Third is additional dependences
# Fourth is whether to install or not
install_plugins_desc = []

# Note that most of these simply append to
# install_plugins_desc.  Some however have
# non-standard compilations and therefore are
# marked as such.
subdir('cairo')
subdir('cgm')
subdir('drs')
subdir('dxf')
subdir('hpgl')
subdir('shape')
subdir('pixbuf')
subdir('layout')  # Non-standard: shared_module
subdir('metapost')
subdir('pdf')
subdir('pgf')
subdir('pstricks')
subdir('python')  # Non-standard: shared_module
subdir('stress')
subdir('svg')
subdir('vdx')
# TODO: Port to Windows.
if host_machine.system() != 'windows'
  subdir('wmf')  # Non-standard: Only installs if libemf present.
endif
subdir('wpg')
subdir('xfig')
subdir('xslt')

foreach p : install_plugins_desc
    message(p.get('name'))
    library(
        p.get('name'),
        p.get('sources'),
        dependencies: [libc_dep, libgtk_dep, libm_dep, libxml_dep, libdia_dep] + [config_dep] + p.get('extra_deps', []),
        install: p.get('install', true),
        install_dir: dialibdir,
        name_suffix: g_module_suffix,
    )
endforeach
