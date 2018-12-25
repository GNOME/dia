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

endif
