
desktop_file = i18n.merge_file(
  input: 'org.gnome.Dia.desktop.in',
  output: '@BASENAME@',
  type: 'desktop',
  po_dir: po_dir,
  install: true,
  install_dir: datadir / 'applications'
)
# Validate Desktop file
desktop_file_validate = find_program('desktop-file-validate', required: false)
if desktop_file_validate.found()
  test(
    'validate-desktop',
    desktop_file_validate,
    args: [
      desktop_file.full_path()
    ]
  )
endif

appstream_file = i18n.merge_file(
  input: 'org.gnome.Dia.metainfo.xml.in',
  output: '@BASENAME@',
  po_dir: po_dir,
  install: true,
  install_dir: datadir / 'metainfo',
)
# Validate Appdata
appstreamcli = find_program(
  'appstreamcli',
  required: get_option('tests'),
)
if appstreamcli.found()
  test(
    'validate-appstream',
    appstreamcli,
    args: [
      'validate',
      '--no-net',
      '--explain',
      appstream_file,
    ],
  )
endif

install_data('org.gnome.Dia.svg',
             install_dir: datadir / 'icons' / 'hicolor' / 'scalable' / 'apps')

install_data('org.gnome.Dia-symbolic.svg',
             install_dir: datadir / 'icons' / 'hicolor' / 'symbolic' / 'apps')

install_data('org.gnome.Dia.thumbnailer',
             install_dir: datadir / 'thumbnailers')

subdir('ui')
