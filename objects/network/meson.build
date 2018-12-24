object_sources = [
    'network.c',
    'bus.c',
    'radiocell.c',
    'basestation.c',
    'wanlink.c'
]

library('network_objects',
    object_sources,
    dependencies : [libgtk_dep, libglib_dep, libxml_dep, libm_dep],
    link_with : libdia,
    include_directories : [configuration_inc, libdia_inc],
    install : true,
    install_dir : dialibdir
)

