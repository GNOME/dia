sources = files(
    'dia-graph.cpp',
    'layout.cpp',
)

deps = [libc_dep, libgtk_dep, libm_dep, libxml_dep]

#TODO: this needs to be tested.
if libogdf_dep.found() == true
    deps += [libogdf_dep]
    sources += ['ogdf-simple.cpp']
endif

# Need to define it as shared module since it depends
# on symbols found in the dia *app* (which will be linked at run-time).
rundeps += shared_module(
    'layout_filter',
    sources,
    dependencies: deps,
    include_directories: [libdia_inc, configuration_inc, diaapp_inc],
    link_with: [libdia],
    install: true,
    install_dir: dialibdir
)
