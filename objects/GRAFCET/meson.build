sources = files(
    'grafcet.c',
    'step.c',
    'action.c',
    'action_text_draw.c',
    'transition.c',
    'boolequation.c',
    'condition.c',
    'vergent.c',
    'vector.c'
)

sources += gnome.compile_resources('dia-grafcet-resources', 'dia-grafcet.gresource.xml',
                                   source_dir : '.',
                                   c_name : 'dia_grafcet')

dia_object_desc += {
    'name': 'grafcet_objects',
    'sources': sources,
}
