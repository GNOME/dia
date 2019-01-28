sources = files(
    'stress.c',
    'stress-memory.c'
)

# This is a development tool for windows.
if host_machine.system() == 'windows'
    install_plugins_desc += {
        'name': 'stress_filter',
        'sources': sources,
        'install': false,
    }
endif
