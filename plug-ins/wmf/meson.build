sources = files('wmf.cpp')

deps = [libemf_dep]

if not libemf_dep.found()
    sources += files('wmf_gdi.cpp')
endif

# TODO: test this on Windows.
if host_machine.system() == 'windows'
    sources += files('paginate_gdiprint.cpp')
    deps += [cc.find_library('comdlg32')]
endif

library(
    'wmf_filter',
    sources,
    dependencies: [libgtk_dep, libm_dep, libxml_dep] + deps,
    include_directories: [libdia_inc, configuration_inc],
    link_with: [libdia],
    install: libemf_dep.found(),
    install_dir: dialibdir
)
