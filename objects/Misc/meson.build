object_sources = [
    'libmisc.c',
    'analog_clock.c',
    'diagram_as_object.c',
    'grid_object.c',
    'measure.c',
    'n_gon.c',
    'tree.c'
]

library('misc_objects',
    object_sources,
    dependencies : [libgtk_dep, libglib_dep, libxml_dep, libm_dep],
    link_with : libdia,
    include_directories : [configuration_inc, libdia_inc],
    install : true,
    install_dir : dialibdir
)

