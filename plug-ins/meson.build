# First element is plugin name
# Second is source list
# Third is additional dependences
# Fourth is whether to install or not
install_plugins_desc = []

subdir('cairo')
subdir('cgm')

# Note that this does not get installed
subdir('drs')

subdir('dxf')
subdir('hpgl')
subdir('shape')
subdir('pixbuf')


# FIXME: requires OGDF
#subdir('layout')

subdir('libart')
subdir('metapost')

# FIXME: does not compile
#subdir('pdf')

subdir('pgf')
#subdir('postscript')

subdir('pstricks')
#subdir('python')
#subdir('stress')

subdir('svg')
# FIXME: requires visio-types.h
#subdir('vdx')

#subdir('wmf')
subdir('wpg')
subdir('xfig')

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
