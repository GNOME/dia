sources = files(
    'dia-xslt-dialogue.c',
    'dia-xslt-dialogue.h',
    'dia-xslt.c',
    'dia-xslt.h',
)

sources += gnome.compile_resources('dia-xslt-resources', 'dia-xslt.gresource.xml',
                                    source_dir : '.',
                                    c_name : 'dia_xslt')

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

install_data(xsls, install_dir: pkgdatadir / 'xslt')

if libxslt_dep.found()
    library(
        'xslt_filter',
        sources,
        dependencies: [libc_dep, libgtk_dep, libm_dep, libxml_dep, libdia_dep, libxslt_dep] + [config_dep],
        install: true,
        install_dir: dialibdir,
        name_suffix: g_module_suffix,
    )
endif
