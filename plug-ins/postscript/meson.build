sources = files(
    'postscript.c',
    'ps-utf8.c',
    'diapsrenderer.c',
    'paginate_psprint.c',
    'render_eps.c',
)
deps = []

if freetype_dep.found()
    sources += files('diapsft2renderer.c')
    deps += [freetype_dep]
endif

# TODO: actual check for w32
platform_is_win32 = false
if platform_is_win32
    sources += files('win32print.c')
    #TODO: add winspool as per Makefile.am
    deps += []
endif

install_plugins_desc += [['postscript_filter', sources, deps]]


