docdir = join_paths(
    get_option('datadir'),
    'doc',
    meson.project_name()
)

dblatex = find_program('dblatex', required: false, disabler: true)
xsltproc = find_program('xsltproc', required: false, disabler: true)

DB2MAN = 'http://docbook.sourceforge.net/release/xsl/current/manpages/docbook.xsl'

helpdir = join_paths(pkgdatadir, 'help')

subdir('de')
subdir('en')
subdir('eu')
subdir('fr')
