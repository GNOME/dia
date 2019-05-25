sources = files(
    'entity.c',
    'relationship.c',
    'attribute.c',
    'participation.c',
    'er.c'
)

sources += gnome.compile_resources('dia-er-resources', 'dia-er.gresource.xml',
                                   source_dir : '.',
                                   c_name : 'dia_er')

dia_object_desc += {
    'name': 'er_objects',
    'sources': sources,
}
