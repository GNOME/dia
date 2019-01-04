# TODO: minimise code duplication.
if host_machine.system() != 'windows'
  bb = executable(
      'boundingbox',
      'test-boundingbox.c',
      dependencies: [libgtk_dep, libxml_dep, libm_dep],
      link_with: [libdia],
      link_args: dia_link_args,
      include_directories: [configuration_inc, libdia_inc],
  )
  objs = executable(
      'objects',
      'test-objects.c',
      dependencies: [libgtk_dep, libxml_dep, libm_dep],
      link_with: [libdia],
      link_args: dia_link_args,
      include_directories: [configuration_inc, libdia_inc],
  )
endif
tsvg = executable(
    'testsvg',
    'test-svg.c',
    dependencies: [libgtk_dep, libxml_dep, libm_dep],
    link_with: [libdia],
    link_args: dia_link_args,
    include_directories: [configuration_inc, libdia_inc],
)
if host_machine.system() != 'windows'
  test('boundingbox', bb)
  test('objects', objs, args: [join_paths(meson.build_root(), 'objects')])
endif
test('testsvg', tsvg)
