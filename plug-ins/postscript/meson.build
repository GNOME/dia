sources = files(
    'postscript.c',
    'ps-utf8.c',
    'diapsrenderer.c',
    'paginate_psprint.c',
    'render_eps.c',
)
deps = []

if host_machine.system() == 'windows'
    sources += files('win32print.c')
    deps += cc.find_library('winspool')
endif

install_plugins_desc += {
    'name': 'postscript_filter',
    'sources': sources,
    'extra_deps': deps,
}
