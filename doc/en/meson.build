# TODO: we need to generate dependecies for dia.xml.
doc_pdf = custom_target('dia.pdf',
    output : 'dia.pdf',
    input  : 'dia.xml',
    command : [dblatex, '-t', 'pdf', '-q', '-o', '@OUTPUT@', '@INPUT@'],
    install : true,
    install_dir : 'doc'
)

xsltproc = find_program('xsltproc')
manpage = custom_target('dia.1',
    output : 'dia.1',
    input  : 'dia.dbk',
    command : [xsltproc, '--novalid', '--output', '@OUTPUT@', DBMAN, '@INPUT@'],
    install : true,
    install_dir : 'doc'
)
