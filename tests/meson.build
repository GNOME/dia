
run_dia = find_program('run_dia.sh')

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
sizeof = executable(
    'sizeof',
    'test-sizeof.c',
    dependencies: [libgtk_dep, libxml_dep, libm_dep],
    link_with: [libdia],
    link_args: dia_link_args,
    include_directories: [configuration_inc, libdia_inc],
)

# Not really a test, but just a helper program.
run_target('sizeof', command: [sizeof])

if host_machine.system() != 'windows'
  test('boundingbox', bb)
  test('objects', objs, args: [join_paths(meson.build_root(), 'objects')])
endif

xmllint_test = find_program('xmllint_test.sh')
render_test_dia = files(join_paths('..', 'samples', 'render-test.dia'))[0]
shape_dtd = files(join_paths('..', 'doc', 'shape.dtd'))[0]

find_program('xmllint', required: true)
test('xmllint',
    xmllint_test,
    args: [
        run_dia.path(),
        shape_dtd,
        render_test_dia
    ],
    env: [
        'MESON_BUILD_ROOT=' + meson.build_root(),
        'MESON_SOURCE_ROOT=' + meson.source_root(),
    ],
)

test('testsvg', tsvg)

subdir('exports')
