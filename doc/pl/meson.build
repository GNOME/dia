xml_deps = files(
    'authors.xml',
    'dia.xml',
    'intro.xml',
    'license.xml',
    'usage-canvas.xml',
    'usage-customization.xml',
    'usage-layers.xml',
    'usage-loadsave.xml',
    'usage-objects-selecting.xml',
    'usage-objects-special.xml',
    'usage-objects.xml',
    'usage-quickstart.xml',
)

html_output = [
    'ch01.html',
    'ch01s02.html',
    'ch01s03.html',
    'ch01s04.html',
    'ch01s05.html',
    'ch01s06.html',
    'ch01s07.html',
    'ch02.html',
    'ch02s02.html',
    'ch02s03.html',
    'ch02s04.html',
    'ch02s05.html',
    'ch03.html',
    'ch03s02.html',
    'ch03s03.html',
    'ch03s04.html',
    'ch04.html',
    'ch04s02.html',
    'ch05.html',
    'ch05s02.html',
    'ch06.html',
    'ch06s02.html',
    'ch07.html',
    'ch08.html',
    'ch08s02.html',
    'ch08s03.html',
    'ch09.html',
    'ch10.html',
    'index.html',
]

lang = 'pl'

if xsltproc.found()
    html_dir = join_paths(helpdir, lang)

    custom_target(lang + '-dia_html',
        input: [ 'html.xsl', 'dia.xml' ],
        output: html_output,
        command: [
            xsltproc,
            '--stringparam', 'graphic.default.extension', 'png',
            '-o', join_paths('@OUTDIR@', ''),
            '@INPUT@'
        ],
        depend_files: xml_deps,
        install: true,
        install_dir: html_dir,
    )

    install_subdir(
        'graphics',
        install_dir: html_dir,
    )

    install_subdir(
        join_paths('..', 'html'),
        install_dir: html_dir,
        strip_directory: true,
    )
endif
