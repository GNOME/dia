if get_option('gtk_doc')

subdir('xml')

private_headers = [
]

images = [
]

content_files = [
]

glib_prefix = dependency('glib-2.0').get_variable('prefix')
glib_docpath = glib_prefix / 'share' / 'gtk-doc' / 'html'
docpath = datadir / 'gtk-doc' / 'html'

gnome.gtkdoc('dia-app',
           main_xml: 'dia-app-docs.xml',
            src_dir: [],
       dependencies: diaapp_dep_for_docs,
  gobject_typesfile: 'dia-app.types',
          scan_args: [
                       '--rebuild-types',
                       '--ignore-headers=' + ' '.join(private_headers),
                     ],
       fixxref_args: [
                       '--html-dir=@0@'.format(docpath),
                       '--extra-dir=@0@'.format(glib_docpath / 'glib'),
                       '--extra-dir=@0@'.format(glib_docpath / 'gobject'),
                       '--extra-dir=@0@'.format(glib_docpath / 'gio'),
                       '--extra-dir=@0@'.format(glib_docpath / 'gtk2'),
                       '--extra-dir=@0@'.format(glib_docpath / 'gdk2'),
                     ],
        install_dir: 'dia-app',
      content_files: content_files,
        html_assets: images,
            install: true)

endif
