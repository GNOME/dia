xml_deps = files(
    'authors.xml',
    'custom-shapes.xml',
    'dia-1.xml',
    'dia-cmdline.xml',
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
    'ch01s02.html',
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
    'ch03s06.html',
    'ch03s07.html',
    'ch04.html',
    'ch04s02.html',
    'ch04s03.html',
    'ch04s04.html',
    'ch04s05.html',
    'ch05.html',
    'ch06.html',
    'ch07.html',
    'ch07s02.html',
    'ch07s03.html',
    'ch08.html',
    'ch08s02.html',
    'ch09.html',
    'ch10.html',
    'ch10s02.html',
    'ch10s03.html',
    'ch11.html',
    'ch11s02.html',
    'ch11s03.html',
    'ch11s04.html',
    'ch11s05.html',
    'ch11s06.html',
    'ch11s07.html',
    'ch11s08.html',
    'ch12.html',
    'ch12s02.html',
    'ch13.html',
    'ch14.html',
    'index.html',
    're01.html',
]

lang = 'en'

custom_target(lang + '-dia.pdf',
    output: 'dia.pdf',
    input: 'dia.xml',
    depend_files: xml_deps,
    command: [dblatex, '-t', 'pdf', '-q', '-o', '@OUTPUT@', '@INPUT@'],
    install: true,
    install_dir: docdir / lang
)

dia_man = custom_target(lang + '-dia.1',
    input: 'dia.dbk',
    output: 'dia.1',
    command: [
        xsltproc, '--novalid',
        '--output', '@OUTPUT@',
        DB2MAN,
        '@INPUT@'
    ],
    install: true,
    install_dir: get_option('mandir') / 'man1'
)
# TODO: this should fixed in meson: https://github.com/mesonbuild/meson/issues/1550
#install_man(dia_man, language: lang)

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
    install_dir: html_dir
)

install_subdir(
    '..' / 'html',
    install_dir: html_dir,
    strip_directory: true
)
