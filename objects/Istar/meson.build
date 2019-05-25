sources = files(
    'istar.c',
    'actor.c',
    'goal.c',
    'other.c',
    'link.c'
)

sources += gnome.compile_resources('dia-istar-resources', 'dia-istar.gresource.xml',
                                   source_dir : '.',
                                   c_name : 'dia_istar')

dia_object_desc += {
    'name': 'istar_objects',
    'sources': sources,
}
