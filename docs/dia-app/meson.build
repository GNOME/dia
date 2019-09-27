if get_option('gtk_doc')

subdir('xml')

private_headers = [
]

images = [
]

content_files = [
]

glib_prefix = dependency('glib-2.0').get_pkgconfig_variable('prefix')
glib_docpath = join_paths(glib_prefix, 'share', 'gtk-doc', 'html')
docpath = join_paths(get_option('datadir'), 'gtk-doc', 'html')

gnome.gtkdoc('dia-app',
           main_xml: 'dia-app-docs.xml',
            src_dir: [
                       diaapp_inc
                     ],
       dependencies: diaapp_dep_for_docs,
  gobject_typesfile: 'dia-app.types',
          scan_args: [
                       '--rebuild-types',
                       '--ignore-headers=' + ' '.join(private_headers),
                     ],
       fixxref_args: [
                       '--html-dir=@0@'.format(docpath),
                       '--extra-dir=@0@'.format(join_paths(glib_docpath, 'glib')),
                       '--extra-dir=@0@'.format(join_paths(glib_docpath, 'gobject')),
                       '--extra-dir=@0@'.format(join_paths(glib_docpath, 'gio')),
                     ],
        install_dir: 'dia-app',
      content_files: content_files,
        html_assets: images,
            install: true)

endif
