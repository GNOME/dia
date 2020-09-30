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

    'dia-builder.c',
    'dia-builder.h',

    'layer-editor/dia-layer-list.c',
    'layer-editor/dia-layer-list.h',
    'layer-editor/dia-layer-widget.c',
    'layer-editor/dia-layer-widget.h',
    'layer-editor/dia-layer-editor.c',
    'layer-editor/dia-layer-editor.h',
    'layer-editor/dia-layer-editor-dialog.c',
    'layer-editor/dia-layer-editor-dialog.h',
    'layer-editor/dia-layer-properties.c',
    'layer-editor/dia-layer-properties.h',
    'layer-editor/layer_dialog.c',
    'layer-editor/layer_dialog.h',

    'dia-guide-tool.c',
    'dia-guide-tool.h',
    'dia-guide-dialog.c',
    'dia-guide-dialog.h',

    'commands.c',
    'app_procs.c',
    'connectionpoint_ops.c',
    'disp_callbacks.c',
    'display.c',
    'select.c',
    'dia-colour-area.c',
    'dia-line-width-area.c',
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
    'ruler.c',
    'tool.c',
    'dia-page-layout.c',
    'pagesetup.c',
    'filedlg.c',
    'find-and-replace.c',
    'plugin-manager.c',
    'dia-props.c',
    'gtkwrapbox.c',
    'gtkhwrapbox.c',

    'cursor.c',
    'splash.c',
    'recent_files.c',

    'dia-application.c',

    'diagram_tree_model.c',
    'diagram_tree_view.c',

    'sheet-editor/sheets.c',
    'sheet-editor/sheets_dialog.c',
    'sheet-editor/sheets_dialog_callbacks.c',
    'sheet-editor/sheet-editor-button.c',
    'sheet-editor/sheet-editor-button.h',

    'autosave.c',
    'dynamic_refresh.c',
    'navigation.c',
    'highlight.c',
    'textedit.c',
    'textedit_tool.c',
]

dia_sources += gnome.mkenums_simple('dia-app-enums',
                                    sources: [
                                        'dia-page-layout.h',
                                        'exit_dialog.h',
                                    ])

#TODO: this is workaround for openbsd where libraries are in /usr/lib/local
# Potential parent issues: https://github.com/mesonbuild/meson/issues/3570
# https://github.com/mesonbuild/meson/issues/4468
pkg_config = find_program('pkg-config')
pkg_config_libdir = run_command(pkg_config, '--libs-only-L', 'libxml-2.0').stdout().strip()

dia_link_args = []
if pkg_config_libdir != ''
    dia_link_args += pkg_config_libdir
endif

diaapp_inc = include_directories('.')

resources = gnome.compile_resources('dia-resources', 'dia.gresource.xml',
                                    source_dir : '.',
                                    c_name : 'dia')

diaapp_lib = both_libraries('diaapp',
                            dia_sources + [diamarshal_h, config_h],
                            dependencies: [libgtk_dep, libxml_dep, libm_dep, libdia_dep, gtk_mac_dep],
                            link_args: dia_link_args,
                            include_directories: [configuration_inc])

diaapp_dep = declare_dependency (sources: [config_h],
                                 include_directories: diaapp_inc,
                                 link_with: diaapp_lib.get_static_lib(),
                                 dependencies: [libgtk_dep, libxml_dep, libm_dep, libdia_dep])

diaapp_dep_for_docs = declare_dependency (include_directories: [diaapp_inc, configuration_inc],
                                          link_with: diaapp_lib.get_shared_lib(),
                                          dependencies: [libgtk_dep, libxml_dep, libm_dep, libdia_dep])

diaapp = executable('dia',
                    ['main.c', resources],
                    dependencies: [diaapp_dep],
                    gui_app: true,
                    export_dynamic: true,  # some plugins require this.
                    install: true)

if build_machine.system() == 'windows'
    dia_win_remote = executable(
        'dia-win-remote',
        ['dia-win-remote.c'],
        dependencies: [libgtk_dep],
        link_args: ['-lshlwapi'],
        gui_app: true,
        install: true,
    )
endif
