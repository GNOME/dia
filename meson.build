project('dia',
        ['c', 'cpp'],
        version : '0.97.3+git' +
            run_command(find_program('meson-helpers/get-git-sha.sh')).stdout().strip(),
        meson_version : '>= 0.45',
        default_options : []
)

add_global_link_arguments('-lc', language: 'c')

cc = meson.get_compiler('c')

#GTK_MODULES="gtk+-2.0 >= 2.16.0 glib-2.0 >= 2.20.0 libxml-2.0 gdk-pixbuf-2.0 gthread-2.0 gmodule-2.0"
libgtk_dep  = dependency('gtk+-2.0', version : '>= 2.16.0')
libglib_dep = dependency('glib-2.0', version : '>= 2.20.0')
libxml_dep  = dependency('libxml-2.0', version : '>= 2.6.27')
#TODO: what are the minimum versions?
gmodule_dep = dependency('gmodule-2.0')
libzlib_dep = dependency('zlib')
#TODO: this is optional.  Should it be a disabler?
libcairo_dep = dependency('cairo', required : false)

libm_dep = cc.find_library('m', required : false)

# Used in pixmap csource generation.
gdk_pixbuf_csource = find_program('gdk-pixbuf-csource')
# TODO
#AC_PROG_INSTALL
#AC_PROG_LN_S
#AC_PROG_MAKE_SET
#AC_ISC_POSIX


datadir = join_paths(get_option('prefix'),
                     get_option('datadir'))

pkgdatadir = join_paths(datadir, meson.project_name())

dialibdir = join_paths(get_option('prefix'),
                       get_option('libdir'),
                       meson.project_name())

# Specify a header configuration file
conf = configuration_data()
conf.set_quoted('VERSION', meson.project_version())
conf.set_quoted('GETTEXT_PACKAGE', meson.project_name())
conf.set_quoted('DATADIR', datadir)
conf.set_quoted('PKGDATADIR', pkgdatadir)
conf.set_quoted('DIALIBDIR',  dialibdir)
conf.set_quoted('LOCALEDIR', get_option('localedir'))
conf.set('ENABLE_NLS', 1)

foreach h : ['stddef.h', 'fcntl.h', 'unistd.h', 'utime.h']
    conf.set10('HAVE_' + h.underscorify().to_upper(), cc.has_header(h))
endforeach

foreach f : ['select', 'strcspn', 'strdup', 'strtol']
    conf.set10('HAVE_' + f.underscorify().to_upper(), cc.has_function(f))
endforeach

configuration_inc = include_directories('.')

# FIXME: I don't think this should be defined.
# To fix we should remove #ifdef HAVE_CONFIG_H checks from all source files.
add_project_arguments('-DHAVE_CONFIG_H', language : 'c')

subdir('po')
subdir('lib')
subdir('objects')
subdir('app')
subdir('data')
subdir('doc')
subdir('plug-ins')
subdir('sheets')
subdir('shapes')

configure_file(output : 'config.h',
               configuration : conf)

meson.add_install_script('meson-helpers/post-install.py', datadir)
