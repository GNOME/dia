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

export_tests = [
    'Arcs',
    'Bezierlines',
    'Ellipses',
    'Polygons',
    'Texts',
    'Beziergons',
    'Boxes',
    'Lines',
    'Polylines',
    'Zigzaglines',
]

diff = find_program('diff')

foreach plugin : export_plugins
    foreach file : export_tests
        output_name = file + '.' + plugin
        actual_output = custom_target(output_name,
           output: file + '.' + plugin,
            input: file + '.dia',
          command: [diaapp, '-t', plugin, '-e', '@OUTPUT@', '@INPUT@'],
              env: run_env,
        )
        test(file, diff,
             args: [
                      '-q', actual_output,
                       meson.current_source_dir() / plugin / file + '.' + plugin
                   ],
            suite: ['export', plugin],
        )
    endforeach
endforeach
