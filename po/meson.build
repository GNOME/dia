i18n = import('i18n')

intltool_merge = find_program('intltool-merge')

po_dir = meson.current_source_dir()

intltool_cache = join_paths(po_dir, '.intltool-merge-cache')
intltool_command = [
    intltool_merge, '-x', '-u',
    '-c', intltool_cache,
    po_dir, '@INPUT@', '@OUTPUT@'
]

i18n.gettext(meson.project_name())
