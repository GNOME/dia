
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

appdata_file = i18n.merge_file (
  input: 'org.gnome.Dia.appdata.xml.in',
  output: '@BASENAME@',
  po_dir: po_dir,
  install: true,
  install_dir: datadir / 'metainfo'
)
# Validate Appdata
appstream_util = find_program('appstream-util', required: false)
if appstream_util.found()
  test(
    'validate-appdata', appstream_util,
    args: [
      'validate', '--nonet', appdata_file.full_path()
    ]
  )
endif

install_data('org.gnome.Dia.svg',
             install_dir: datadir / 'icons' / 'hicolor' / 'scalable' / 'apps')

install_data('org.gnome.Dia-symbolic.svg',
             install_dir: datadir / 'icons' / 'hicolor' / 'symbolic' / 'apps')

install_data('org.gnome.Dia.thumbnailer',
             install_dir: datadir / 'thumbnailers')

subdir('ui')
