plugin_sources = [
	'dxf.c',
	'dxf-export.c',
	'dxf-import.c',
	'autocad_pal.c'
]

shared_library(
    'dxf_filter',
    plugin_sources,
    dependencies : [ libgtk_dep,
                     libxml_dep,
                     libm_dep ],
    include_directories : [ libdia_inc,
                            configuration_inc ],
    link_with : [ libdia ],
    install : true,
    install_dir : dialibdir
)
