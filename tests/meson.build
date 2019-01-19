# Setup local environment to run dia.
run_env_dict = {
    'DIA_BASE_PATH'  : [meson.source_root()],
    'DIA_LIB_PATH'   : [join_paths(meson.build_root(), 'objects'),
                        join_paths(meson.build_root(), 'plug-ins')],
    'DIA_SHAPE_PATH' : [join_paths(meson.source_root(), 'shapes')],
    'DIA_XSLT_PATH'  : [join_paths(meson.source_root(), 'plug-ins', 'xslt')],
    'DIA_PYTHON_PATH': [join_paths(meson.source_root(), 'plug-ins', 'python')],
    'DIA_SHEET_PATH' : [join_paths(meson.build_root(), 'sheets')],
}

run_env = environment()
foreach k, v : run_env_dict
    run_env.set(k, v)
endforeach
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
