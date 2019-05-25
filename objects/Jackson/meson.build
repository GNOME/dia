sources = files(
    'jackson.c',
    'domain.c',
    'requirement.c',
    'phenomenon.c'
)

sources += gnome.compile_resources('dia-jackson-resources', 'dia-jackson.gresource.xml',
                                   source_dir : '.',
                                   c_name : 'dia_jackson')

dia_object_desc += {
    'name': 'jackson_objects',
    'sources': sources,
}
