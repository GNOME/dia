plugin_sources = [
    'diacairo.c',
    'diacairo-interactive.c',
    'diacairo-renderer.c',
    'diacairo-print.c'
]

shared_library(
    'cairo_filter',
    plugin_sources,
    dependencies : [ libcairo_dep,
                     libgtk_dep,
                     libxml_dep,
                     libm_dep ],
    include_directories : [ libdia_inc,
                            configuration_inc ],
    link_with : [ libdia ],
    install : true,
    install_dir : dialibdir
)
