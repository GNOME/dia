sources = [files('pdf.c')]
deps = []

if libpoppler_dep.found() and libpopplercpp_dep.found()
    sources += files('pdf-import.cpp')
    deps += libpoppler_dep
    deps += libpopplercpp_dep
endif

install_plugins_desc += {
    'name': 'pdf_filter',
    'sources': sources,
    'extra_deps': deps,
}
