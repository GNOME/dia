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
shared_module(
    'layout_filter',
    sources,
    dependencies: [deps, libdia_dep] + [config_dep],
    link_with: [diaapp], # Naughty
    install: true,
    install_dir: dialibdir,
    include_directories: diaapp_inc,
    name_suffix: g_module_suffix,
)
