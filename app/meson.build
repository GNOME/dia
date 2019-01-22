subdir('pixmaps')

dia_sources = [
    'confirm.c',
    'exit_dialog.c',
    'diagram.c',
    'preferences.c',
    'load_save.c',
    'properties-dialog.c',
    'defaults.c',
    'undo.c',
    'object_ops.c',
    'layer_dialog.c',
    'commands.c',
    'app_procs.c',
    'connectionpoint_ops.c',
    'disp_callbacks.c',
    'display.c',
    'select.c',
    'color_area.c',
    'linewidth_area.c',
    'grid.c',
    'handle_ops.c',
    'interface.c',
    'toolbox.c',
    'modify_tool.c',
    'scroll_tool.c',
    'create_object.c',
    'magnify.c',
    'menus.c',
    'cut_n_paste.c',
    'render_gdk.c',
    'ruler.c',
    'tool.c',
    'diapagelayout.c',
    'pagesetup.c',
    'filedlg.c',
    'find-and-replace.c',
    'plugin-manager.c',
    'dia-props.c',
    'gtkwrapbox.c',
    'gtkhwrapbox.c',
    'gtkvwrapbox.c',
    'cursor.c',
    'splash.c',
    'recent_files.c',

    'dia-application.c',

    'diagram_tree_model.c',
    'diagram_tree_view.c',

    'sheets.c',
    'sheets_dialog.c',
    'sheets_dialog_callbacks.c',
    'autosave.c',
    'dynamic_refresh.c',
    'navigation.c',
    'highlight.c',
    'textedit.c',
    'textedit_tool.c',

    'main.c'
]

#TODO: this is workaround for openbsd where libraries are in /usr/lib/local
pkg_config = find_program('pkg-config')
pkg_config_libdir = run_command(pkg_config, '--libs-only-L', 'libxml-2.0').stdout().strip()

dia_link_args = []
if pkg_config_libdir != ''
    dia_link_args += pkg_config_libdir
endif

diaapp_inc = include_directories('.')

diaapp = executable('dia',
    dia_sources + [dia_app_icons_h, diamarshal_h, config_h],
    dependencies: [libgtk_dep, libxml_dep, libm_dep],
    link_with: [libdia],
    link_args: dia_link_args,
    export_dynamic: true,  # some plugins require this.
    include_directories: [configuration_inc, libdia_inc],
    gui_app: true,
    install: true,
)
