sources = files(
    'dia-render-script.c',
    'dia-render-script-import.c',
    'dia-render-script-object.c',
    'dia-render-script-renderer.c'
)

# Prevent installation - currently a debug tool only
shared_library(
    'drs_filter',
    sources,
    dependencies: [libgtk_dep, libm_dep, libxml_dep],
    include_directories: [libdia_inc, configuration_inc],
    link_with: [libdia],
    install: false,
    install_dir: dialibdir
)
