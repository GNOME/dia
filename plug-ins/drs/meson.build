plugin_sources = [
    'dia-render-script.c',
    'dia-render-script-import.c',
    'dia-render-script-object.c',
    'dia-render-script-renderer.c'
]

shared_library(
    'drs_filter',
    plugin_sources,
    dependencies : [ libgtk_dep,
                     libxml_dep,
                     libm_dep ],
    include_directories : [ libdia_inc,
                            configuration_inc ],
    link_with : [ libdia ],
    install : false,   # Prevent installation - currently a debug tool only
    install_dir : dialibdir
)
