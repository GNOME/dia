sources = files(
    'xslt.c',
    'xsltdialog.c'
)

xsls = files(
    'dia-uml.xsl',
    'dia-uml2c++.xsl',
    'dia-uml2cpp.xsl',
    'dia-uml2haxe.xsl',
    'dia-uml2java.xsl',
    'dia-uml2idl.xsl',
    'dia-uml2python.xsl',
    'dia-uml2owl.xsl',
    'stylesheets.xml',
    'dia-uml2componentlist.xsl',
)

install_data(
    xsls,
    install_dir: join_paths(pkgdatadir, 'xslt')
)

if libxslt_dep.found()
    install_plugins_desc += {
        'name': 'xslt_filter',
        'sources': sources,
        'extra_deps': [libxslt_dep],
    }
endif
