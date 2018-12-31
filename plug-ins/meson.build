# First element is plugin name
# Second is source list
# Third is additional dependences
# Fourth is whether to install or not
install_plugins_desc = []

subdir('cairo')
subdir('cgm')
subdir('drs')
subdir('dxf')
subdir('hpgl')

# TODO: enable or remove these.
#subdir('layout')
#subdir('libart')

#subdir('metapost')
#subdir('pdf')
#subdir('pgf')
#subdir('pixbuf')
#subdir('postscript')
#subdir('pstricks')
#subdir('python')
#subdir('shape')
#subdir('stress')
#subdir('svg')
#subdir('vdx')
#subdir('wmf')
#subdir('wpg')
#subdir('xfig')
#subdir('xslt')

foreach p : install_plugins_desc
    message(p[0])
    shared_library(
        p[0],
        p[1],
        dependencies: [libgtk_dep, libm_dep, libxml_dep] + [p[2]],
        include_directories: [libdia_inc, configuration_inc],
        link_with: [libdia],
        install: true,
        install_dir: dialibdir
    )
endforeach
