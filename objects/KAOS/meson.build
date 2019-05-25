sources = files(
    'kaos.c',
    'goal.c',
    'other.c',
    'metabinrel.c',
    'metaandorrel.c'
)

sources += gnome.compile_resources('dia-kaos-resources', 'dia-kaos.gresource.xml',
                                   source_dir : '.',
                                   c_name : 'dia_kaos')

dia_object_desc += {
    'name': 'kaos_objects',
    'sources': sources,
}
