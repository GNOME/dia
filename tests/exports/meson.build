exports_test = find_program('exports_test.sh')

test('exports',
    exports_test,
    args: [run_dia.path()],
    workdir: meson.current_source_dir(),
    env: [
        'MESON_BUILD_ROOT=' + meson.build_root(),
        'MESON_SOURCE_ROOT=' + meson.source_root(),
    ],
)
