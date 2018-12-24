object_sources = [
    'kaos.c',
    'goal.c',
    'other.c',
    'metabinrel.c',
    'metaandorrel.c'
]

library('kaos_objects',
    object_sources,
    dependencies : [libgtk_dep, libglib_dep, libxml_dep, libm_dep],
    link_with : libdia,
    include_directories : [configuration_inc, libdia_inc],
    install : true,
    install_dir : dialibdir
)

