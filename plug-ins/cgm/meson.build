shared_library(
    'cgm_filter',
    'cgm.c',
    dependencies : [ libgtk_dep,
                     libm_dep ],
    include_directories : [ libdia_inc,
                            configuration_inc ],
    link_with : [ libdia ],
    install : true,
    install_dir : dialibdir
)
