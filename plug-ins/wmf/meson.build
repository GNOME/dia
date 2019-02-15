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

install_plugins_desc += {
    'name': 'wmf_filter',
    'sources': sources,
    'extra_deps': deps,
    'install': libemf_dep.found(),
}
