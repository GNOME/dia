object_sources = [
    'shape_info.c',
    'shape_typeinfo.c',
    'custom_object.c',
    'custom_util.c',
    'custom.c'
]

library('custom_objects',
    object_sources,
    dependencies : [libgtk_dep, libglib_dep, libxml_dep, libm_dep],
    link_with : libdia,
    include_directories : [configuration_inc, libdia_inc],
    install : true,
    install_dir : dialibdir
)

