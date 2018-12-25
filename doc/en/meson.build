docdir = join_paths(
    get_option('datadir'),
    'doc',
    meson.project_name()
)

dia_xml_deps = [
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
]

if dblatex.found()
    custom_target('dia.pdf',
        output : 'dia.pdf',
        input  : 'dia.xml',
        depend_files : dia_xml_deps,
        command : [dblatex, '-t', 'pdf', '-q', '-o', '@OUTPUT@', '@INPUT@'],
        install : true,
        install_dir : docdir
    )
endif

dia_html_output = [
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

if xsltproc.found()
    dia_man = custom_target('dia.1',
        output : 'dia.1',
        input  : 'dia.dbk',
        command : [xsltproc, '--novalid', '--output', '@OUTPUT@', DB2MAN, '@INPUT@'],
        install : true,
        install_dir : join_paths(get_option('mandir'), 'man1')
    )
    # XXX: this should fixed in meson: https://github.com/mesonbuild/meson/issues/1550
    #install_data(dia_man)

    html_dir = join_paths(helpdir, 'en')

    custom_target('dia_html',
        input : ['html.xsl', 'dia.xml'],
        command : [xsltproc, '--stringparam', 'graphic.default.extension', 'png', '-o', join_paths('@OUTDIR@', ''), '@INPUT@'],
        output : dia_html_output,
        depend_files : dia_xml_deps,
        install : true,
        install_dir : html_dir,
    )

    install_subdir(
        'graphics',
        install_dir : html_dir
    )

    install_subdir(
        join_paths('..', 'html'),
        install_dir : html_dir,
        strip_directory : true
    )
endif
