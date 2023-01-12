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
    dependencies: [libc_dep, libgtk_dep, libm_dep, libxml_dep, libdia_dep] + deps + [config_dep],
    install: libemf_dep.found(),
    install_dir: dialibdir,
    name_suffix: g_module_suffix,
)
