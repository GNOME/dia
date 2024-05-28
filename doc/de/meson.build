xml_deps = files(
    'authors.xml',
    'dia.xml',
    'intro.xml',
    'license-documentation.xml',
    'license.xml',
    'usage-canvas.xml',
    'usage-loadsave.xml',
    'usage-objects-basic.xml',
    'usage-objects-selecting.xml',
    'usage-objects-special.xml',
    'usage-objects.xml',
    'usage-quickstart.xml',
)

html_output = [
    'apa.html',
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
    'index.html',
]

lang = 'de'

# TODO: this requires tex-live-german
# https://tex.stackexchange.com/questions/139700/package-babel-error-unknown-option-francais
#custom_target(lang + '-dia.pdf',
#    output : 'dia.pdf',
#    input  : 'dia.xml',
#    depend_files : xml_deps,
#    command : [dblatex, '-t', 'pdf', '-q', '-o', '@OUTPUT@', '@INPUT@'],
#    install : true,
#    install_dir : join_paths(docdir, lang)
#)

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
