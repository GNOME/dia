# Setup local environment to run dia.
run_env_dict = {
    'DIA_BASE_PATH'  : [meson.source_root()],
    'DIA_LIB_PATH'   : [meson.build_root() / 'objects',
                        meson.build_root() / 'plug-ins'],
    'DIA_SHAPE_PATH' : [meson.source_root() / 'shapes'],
    'DIA_XSLT_PATH'  : [meson.source_root() / 'plug-ins' / 'xslt'],
    'DIA_PYTHON_PATH': [meson.source_root() / 'plug-ins' / 'python'],
    'DIA_SHEET_PATH' : [meson.build_root() / 'sheets'],
}

run_env = environment()
foreach k, v : run_env_dict
    run_env.set(k, v)
endforeach

# Otherwise Windows won't be able to find libdia.dll in build directory.
if host_machine.system() == 'windows'
    run_env.append('PATH', meson.build_root() / 'lib')
endif

test_exes = []
foreach t : ['boundingbox', 'objects', 'svg', 'sizeof']
    test_exes += [
        executable(
            'test-' + t,
            ['test-' + t + '.c', config_h],
            dependencies: [libgtk_dep, libxml_dep, libm_dep, libdia_dep],
            link_args: dia_link_args,
            include_directories: [configuration_inc],
        )
    ]
endforeach

# Unfortunately dictionaries can only contain literal strings.
# As such we have to use indeces instead.
test('boundinbox', test_exes[0])
test('objects', test_exes[1], args: [meson.build_root() / 'objects'])
test('testsvg', test_exes[2])

# Not really a test, but just a helper program.
run_target('sizeof', command: [test_exes[3]])

xmllint_test = find_program('xmllint_test.sh')
render_test_dia = files('..' / 'samples' / 'render-test.dia')[0]
shape_dtd = files('..' / 'doc' / 'shape.dtd')[0]

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

subdir('exports')
