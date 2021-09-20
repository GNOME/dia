export_plugins = [
    'cgm',
    'dxf',
    'eps',
    'eps-builtin',
    'fig',
    'mp',
    'plt',
    'hpgl',
    'png',
    'jpg',
    'shape',
    'svg',
    'tex',
    'wpg',
]

export_script = find_program('exports_test.sh')

foreach plugin : export_plugins
    test('exports-' + plugin,
        export_script,
        args: [
            # TODO: We don't need to specify full_path().  See
            # https://github.com/mesonbuild/meson/issues/2681
            diaapp.full_path(),
            plugin,
        ],
        workdir: meson.current_source_dir(),
        env: run_env,
    )
endforeach
