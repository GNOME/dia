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

gnome.gtkdoc('dia',
           main_xml: 'dia-docs.xml',
            src_dir: [
                       libdia_inc
                     ],
       dependencies: libdia_dep,
  gobject_typesfile: 'dia.types',
          scan_args: [
                       '--rebuild-types',
                       '--ignore-headers=' + ' '.join(private_headers),
                     ],
       fixxref_args: [
                       '--html-dir=@0@'.format(docpath),
                       '--extra-dir=@0@'.format(glib_docpath / 'glib'),
                       '--extra-dir=@0@'.format(glib_docpath / 'gobject'),
                       '--extra-dir=@0@'.format(glib_docpath / 'gio'),
                     ],
        install_dir: 'dia',
      content_files: content_files,
        html_assets: images,
            install: true)

endif
