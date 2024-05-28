xml_deps = files(
    'authors.xml',
    'dia-1.xml',
    'dia.xml',
    'entities.xml',
    'intro.xml',
    'license.xml',
    'usage-canvas.xml',
    'usage-customization.xml',
    'usage-layers.xml',
    'usage-loadsave.xml',
    'usage-objects-basic.xml',
    'usage-objects-selecting.xml',
    'usage-objects-special.xml',
    'usage-objects.xml',
    'usage-quickstart.xml',
)

html_output = [
    'ch01.html',
    'ch02.html',
    'ch02s02.html',
    'ch02s03.html',
    'ch02s04.html',
    'ch02s05.html',
    'ch02s06.html',
    'ch02s07.html',
    'ch03.html',
    'ch03s02.html',
    'ch03s03.html',
    'ch03s04.html',
    'ch03s05.html',
    'ch04.html',
    'ch04s02.html',
    'ch04s03.html',
    'ch04s04.html',
    'ch05.html',
    'ch06.html',
    'ch06s02.html',
    'ch07.html',
    'ch07s02.html',
    'ch08.html',
    'ch08s02.html',
    'ch09.html',
    'ch10.html',
    'ch10s02.html',
    'ch10s03.html',
    'ch11.html',
    'ch12.html',
    'index.html',
]

lang = 'eu'

html_dir = helpdir / lang

custom_target(lang + '-dia_html',
    input: [ 'html.xsl', 'dia.xml' ],
    output: html_output,
    command: [
        xsltproc,
        '--stringparam', 'graphic.default.extension', 'png',
        '--output', '@OUTDIR@/',
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
    '..' / 'html',
    install_dir: html_dir,
    strip_directory: true,
)
