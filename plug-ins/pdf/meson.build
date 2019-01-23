sources = [files('pdf.c')]
deps = []

# TODO: we should be able to use has_header_symbol.
# However, that does not seem to pick up on GBool, gTrue etc.
#has_gTrue = cc.has_header_symbol(
#    'goo/gtypes.h',
#    'GBool',
#    dependencies: libpoppler_dep
#)
has_gtypes = cc.has_header(
    'goo/gtypes.h',
    dependencies: libpoppler_dep
)

if libpoppler_dep.found() and has_gtypes
    sources += files('pdf-import.cpp')
    deps += libpoppler_dep
endif

install_plugins_desc += {
    'name': 'pdf_filter',
    'sources': sources,
    'extra_deps': deps,
}
