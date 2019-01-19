# Setup local environment to run dia.
run_env = environment()
run_env.set('DIA_BASE_PATH', meson.source_root())
run_env.append('DIA_LIB_PATH',
    join_paths(meson.build_root(), 'objects'),
    join_paths(meson.build_root(), 'plug-ins'))
run_env.set('DIA_SHAPE_PATH',  join_paths(meson.source_root(), 'shapes'))
run_env.set('DIA_XSLT_PATH',   join_paths(meson.source_root(), 'plug-ins', 'xslt'))
run_env.set('DIA_PYTHON_PATH', join_paths(meson.source_root(), 'plug-ins', 'python'))
run_env.set('DIA_SHEET_PATH',  join_paths(meson.build_root(), 'sheets'))

# TODO: minimise code duplication.
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

# Otherwise Windows won't be able to find libdia.dll in build directory.
if host_machine.system() == 'windows'
    run_env.append('PATH', join_paths(meson.build_root(), 'lib'))
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

test('boundingbox', bb)
test('objects', objs, args: [join_paths(meson.build_root(), 'objects')])

xmllint_test = find_program('xmllint_test.sh')
render_test_dia = files(join_paths('..', 'samples', 'render-test.dia'))[0]
shape_dtd = files(join_paths('..', 'doc', 'shape.dtd'))[0]

find_program('xmllint', required: true)
test('xmllint',
    xmllint_test,
    args: [
        diaapp,
        shape_dtd,
        render_test_dia
    ],
    env: run_env,
)

test('testsvg', tsvg)

subdir('exports')
