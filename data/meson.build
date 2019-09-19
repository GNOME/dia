
desktop_file = i18n.merge_file(
  input: 'org.gnome.Dia.desktop.in',
  output: '@BASENAME@',
  type: 'desktop',
  po_dir: po_dir,
  install: true,
  install_dir: join_paths(datadir, 'applications')
)
# Validate Desktop file
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
  install_dir: join_paths(datadir, 'metainfo')
)
# Validate Appdata
if appstream_util.found()
  test(
    'validate-appdata', appstream_util,
    args: [
      'validate', '--nonet', appdata_file.full_path()
    ]
  )
endif


subdir('icons')
subdir('ui')
