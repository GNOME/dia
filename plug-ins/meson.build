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
subdir('drs')  # Non-standard: does not get installed.
subdir('dxf')
subdir('hpgl')
subdir('shape')
subdir('pixbuf')
subdir('layout')  # Non-standard: shared_module
subdir('libart')
subdir('metapost')
subdir('pdf')
subdir('pgf')
subdir('postscript')
subdir('pstricks')
subdir('python')  # Non-standard: shared_module
subdir('stress')  # Non-standard: does not get installed.
subdir('svg')
subdir('vdx')
#TODO: same here.
if host_machine.system() != 'windows'
  subdir('wmf')  # Non-standard: Only installs if libemf present.
endif
subdir('wpg')
subdir('xfig')
subdir('xslt')

foreach p : install_plugins_desc
    message(p[0])
    library(
        p[0],
        p[1],
        dependencies: [libc_dep, libgtk_dep, libm_dep, libxml_dep] + [p[2]],
        include_directories: [libdia_inc, configuration_inc],
        link_with: [libdia],
        install: true,
        install_dir: dialibdir
    )
endforeach
