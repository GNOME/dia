test_env = {
  'G_DEBUG': 'gc-friendly,fatal-warnings',
  'G_TEST_BUILDDIR': meson.current_build_dir(),
  'G_TEST_SRCDIR': meson.current_source_dir(),
  'GSETTINGS_BACKEND': 'memory',
  'GSETTINGS_SCHEMA_DIR': meson.project_build_root() / 'data',
  'GTK_A11Y': 'test',
  'MALLOC_CHECK_': '2',
  'NO_AT_BRIDGE': '1',
  'PYTHONDEVMODE': '1',
  'PYTHONDONTWRITEBYTECODE': 'yes',
} + run_env_dict

test_exes = []
foreach t : ['boundingbox', 'objects', 'svg', 'sizeof']
  test_exes += [
    executable(
      'test-' + t,
      ['test-' + t + '.c'],
      dependencies: [
        libgtk_dep,
        libxml_dep,
        libm_dep,
        libdia_dep,
      ] + [
        config_dep,
      ],
      link_args: dia_link_args,
    ),
  ]
endforeach

# Unfortunately dictionaries can only contain literal strings.
# As such we have to use indeces instead.
test('boundinbox', test_exes[0], env: test_env)
test(
  'objects',
  test_exes[1],
  args: [meson.global_build_root() / 'objects'],
  env: test_env,
)
test('testsvg', test_exes[2], env: test_env)

# Not really a test, but just a helper program.
run_target('sizeof', command: [test_exes[3]])

xmllint_test = find_program('xmllint_test.sh')
render_test_dia = files('..' / 'samples' / 'render-test.dia')[0]
shape_dtd = files('..' / 'doc' / 'shape.dtd')[0]

find_program('xmllint', required: true)
test(
  'xmllint',
  xmllint_test,
  args: [diaapp, shape_dtd, render_test_dia],
  env: run_env,
)


diff = find_program('diff')
export_test = executable(
  'test-export',
  'test-export.c',
  dependencies: [gio_dep, config_dep],
  c_args: [
    '-DDIFF_BIN="@0@"'.format(diff.full_path()),
    '-DDIA_BIN="@0@"'.format(diaapp.full_path()),
  ],
)

# Most of these tests are currently broken
# test('export', export_test, env: test_env, protocol: 'tap')
