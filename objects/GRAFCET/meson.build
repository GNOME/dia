object_sources = [
    'grafcet.c',
    'step.c',
    'action.c',
    'action_text_draw.c',
    'transition.c',
    'boolequation.c',
    'condition.c',
    'vergent.c',
    'vector.c'
]

library('grafcet_objects',
    object_sources,
    dependencies : [libgtk_dep, libglib_dep, libxml_dep, libm_dep],
    link_with : libdia,
    include_directories : [configuration_inc, libdia_inc],
    install : true,
    install_dir : dialibdir
)

