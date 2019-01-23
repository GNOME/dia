test('exports',
    find_program('exports_test.sh'),
    args: [
        # TODO: We don't need to specify full_path().  See
        # https://github.com/mesonbuild/meson/issues/2681
        diaapp.full_path()
    ],
    workdir: meson.current_source_dir(),
    env: run_env,
)
