# This depends on WITH_SOZI compilation option.
if false
    object_sources = [
        'sozi.c',
        'sozi-object.c',
        'sozi-frame.c',
        'sozi-media.c'
    ]

    library('sozi_objects',
        object_sources,
        dependencies : [libgtk_dep, libglib_dep, libxml_dep, libm_dep],
        link_with : libdia,
        include_directories : [configuration_inc, libdia_inc],
        install : true,
        install_dir : dialibdir
    )
endif
