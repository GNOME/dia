object_sources = [
    'aadl.c',
    'aadlbox.c',
    'aadldata.c',
    'aadlport.c',
    'aadlbus.c',
    'aadldevice.c',
    'aadlmemory.c',
    'aadlprocess.c',
    'aadlprocessor.c',
    'aadlsubprogram.c',
    'aadlsystem.c',
    'aadltext.c',
    'aadlthread.c',
    'aadlthreadgroup.c',
    'aadlpackage.c',
    'edit_port_declaration.c',
]

library('aadl_objects',
    object_sources,
    dependencies : [libgtk_dep, libglib_dep, libxml_dep, libm_dep],
    link_with : libdia,
    include_directories : [configuration_inc, libdia_inc],
    install : true,
    install_dir : dialibdir
)

