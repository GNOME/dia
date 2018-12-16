project('dia',
        ['c', 'cpp'],
        version : '0.97.3+git' +
            run_command(find_program('meson-helpers/get-git-sha.sh')).stdout().strip(),
        meson_version : '>= 0.45',
        default_options : []
)

# TODO
#AC_PROG_INSTALL
#AC_PROG_LN_S
#AC_PROG_MAKE_SET
#AC_ISC_POSIX
cc = meson.get_compiler('c')

#AC_HEADER_STDC
#TODO: do w eneed to check for this?
#AC_CHECK_HEADERS(stddef.h fcntl.h unistd.h utime.h)
check_headers = ['stddef.h', 'fcntl.h', 'unistd.h', 'utime.h']

foreach h : check_headers
    #TODO: documentation we should use dependency instead of has_header.
    assert(cc.has_header(h), h + ' required')
endforeach

#TODO: do we need to check for this?
#AC_C_CONST
#AC_CHECK_FUNCS(select strcspn strdup strtol)
check_funcs = ['select', 'strcspn', 'strdup', 'strtol']
foreach f : check_funcs
    assert(cc.has_function(f), f + ' is required')
endforeach

# Specify a header configuration file
conf = configuration_data()

prefix = get_option('prefix')
conf.set_quoted('VERSION', meson.project_version())

configure_file(output : 'config.h',
               configuration : conf)


