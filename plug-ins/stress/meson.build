# This is currently windows+development only
# It should be easy to port but never get installed

sources = files(
    'stress.c',
    'stress-memory.c'
)

# TODO: test this on Windows.
if host_machine.system() != 'windows'
    library(
        'stress_filter',
        sources,
        dependencies: [libc_dep, libgtk_dep, libm_dep],
        include_directories: [libdia_inc, configuration_inc],
        link_with: [libdia],
        install: false,
        install_dir: dialibdir
    )
endif
