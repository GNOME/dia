sources = files(
    'uml.c',
    'class.c',
    'class_dialog.c',
    'class_attributes_dialog.c',
    'class_operations_dialog.c',
    'class_templates_dialog.c',
    'note.c',
    'actor.c',
    'usecase.c',
    'realizes.c',
    'constraint.c',
    'small_package.c',
    'large_package.c',
    'implements.c',
    'generalization.c',
    'association.c',
    'dependency.c',
    'message.c',
    'object.c',
    'lifeline.c',
    'component.c',
    'component_feature.c',
    'classicon.c',
    'state.c',
    'activity.c',
    'state_term.c',
    'node.c',
    'branch.c',
    'fork.c',
    'stereotype.c',
    'transition.c',
    'umlattribute.c',
    'umloperation.c',
    'umlparameter.c',
    'umlformalparameter.c'
)

sources += gnome.compile_resources('dia-uml-resources', 'dia-uml.gresource.xml',
                                   source_dir : '.',
                                   c_name : 'dia_uml')

sources += gnome.mkenums_simple('dia-uml-enums', sources: ['uml.h'])

dia_object_desc += {
    'name': 'uml_objects',
    'sources': sources,
    'inc': include_directories('.'),
}
