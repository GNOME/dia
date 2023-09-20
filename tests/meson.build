test_exes = []
foreach t : ['boundingbox', 'objects', 'svg', 'sizeof']
    test_exes += [
        executable(
            'test-' + t,
            ['test-' + t + '.c'],
            dependencies: [libgtk_dep, libxml_dep, libm_dep, libdia_dep] + [config_dep],
            link_args: dia_link_args,
        )
    ]
endforeach

# Unfortunately dictionaries can only contain literal strings.
# As such we have to use indeces instead.
test('boundinbox', test_exes[0])
test('objects', test_exes[1], args: [meson.global_build_root() / 'objects'])
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
