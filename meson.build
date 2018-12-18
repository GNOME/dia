project('dia',
        ['c', 'cpp'],
        version : '0.97.3+git' +
            run_command(find_program('meson-helpers/get-git-sha.sh')).stdout().strip(),
        meson_version : '>= 0.45',
        default_options : []
)

i18n = import('i18n')

gettext_package = 'dia'
subdir('po')

cc = meson.get_compiler('c')

#GTK_MODULES="gtk+-2.0 >= 2.16.0 glib-2.0 >= 2.20.0 libxml-2.0 gdk-pixbuf-2.0 gthread-2.0 gmodule-2.0"
libgtk_dep  = dependency('gtk+-2.0', version : '>= 2.16.0')
libglib_dep = dependency('glib-2.0', version : '>= 2.20.0')
libxml_dep  = dependency('libxml-2.0')
gmodule_dep = dependency('gmodule-2.0')
libm_dep = cc.find_library('m', required : false)

libzlib_dep = dependency('zlib')


# Used in pixmap csource generation.
gdk_pixbuf_csource = find_program('gdk-pixbuf-csource')
# TODO
#AC_PROG_INSTALL
#AC_PROG_LN_S
#AC_PROG_MAKE_SET
#AC_ISC_POSIX

# Specify a header configuration file
conf = configuration_data()

datadir = join_paths(get_option('prefix'),
                     get_option('datadir'))

pkgdatadir = join_paths(datadir, 'dia')

conf.set_quoted('VERSION', meson.project_version())
conf.set_quoted('GETTEXT_PACKAGE', 'dia')
conf.set_quoted('DATADIR', datadir)
conf.set_quoted('PKGDATADIR', pkgdatadir)
conf.set_quoted('LIBDIR',  join_paths(get_option('prefix'), get_option('libdir')))
conf.set_quoted('LOCALEDIR', get_option('localedir'))
conf.set('ENABLE_NLS', 1)

foreach h : ['stddef.h', 'fcntl.h', 'unistd.h', 'utime.h']
    conf.set10('HAVE_' + h.underscorify().to_upper(), cc.has_header(h))
endforeach

foreach f : ['select', 'strcspn', 'strdup', 'strtol']
    conf.set10('HAVE_' + f.underscorify().to_upper(), cc.has_function(f))
endforeach

configure_file(output : 'config.h',
               configuration : conf)
configuration_inc = include_directories('.')

# FIXME: I don't think this should be defined.
# To fix we should remove #ifdef HAVE_CONFIG_H checks from all source files.
add_project_arguments('-DHAVE_CONFIG_H', language : 'c')

subdir('lib')
subdir('objects')
subdir('app')
subdir('data')
subdir('doc')
