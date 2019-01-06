project('dia',
        ['c', 'cpp'],
        version : '0.97.3+git' +
            run_command(find_program('meson-helpers/get-git-sha.sh')).stdout().strip(),
        meson_version : '>= 0.49',
        default_options : []
)

cc = meson.get_compiler('c')
conf = configuration_data()

#GTK_MODULES="gtk+-2.0 >= 2.16.0 glib-2.0 >= 2.20.0 libxml-2.0 gdk-pixbuf-2.0 gthread-2.0 gmodule-2.0"
libgtk_dep  = dependency('gtk+-2.0', version : '>= 2.16.0')
libglib_dep = dependency('glib-2.0', version : '>= 2.20.0')
libxml_dep  = dependency('libxml-2.0', version : '>= 2.6.27')

#TODO: what are the minimum versions?
gmodule_dep = dependency('gmodule-2.0')
libzlib_dep = dependency('zlib')
libcairo_dep = dependency('cairo')
#TODO: this will become a hard dependency as part of GTK3 port.
# Therefore, remove all references to HAVE_CAIRO
conf.set('HAVE_CAIRO', 1)
libm_dep = cc.find_library('m')
libc_dep = cc.find_library('c', required : false)

# Optional deps
freetype_dep = dependency('freetype2', version: ['>= 11.0.5', '< 22.0.0'], required: false)
conf.set('HAVE_FREETYPE', freetype_dep.found())
libpoppler_dep = dependency('poppler', version: '<= 0.62.0', required: false)
conf.set('HAVE_POPPLER', libpoppler_dep.found())
libemf_dep = cc.find_library('emf', required: false)
conf.set('HAVE_LIBEMF', libemf_dep.found())
libogdf_dep = cc.find_library('ogdf', required: false)
conf.set('HAVE_OGDF', libogdf_dep.found())
libxslt_dep = dependency('libxslt', required: false)
conf.set('HAVE_XSLT', libxslt_dep.found())

# Used in pixmap csource generation.
gdk_pixbuf_csource = find_program('gdk-pixbuf-csource')

datadir = join_paths(get_option('prefix'),
                     get_option('datadir'))

pkgdatadir = join_paths(datadir, meson.project_name())

dialibdir = join_paths(get_option('prefix'),
                       get_option('libdir'),
                       meson.project_name())

# Specify a header configuration file
conf.set_quoted('VERSION', meson.project_version())
conf.set_quoted('GETTEXT_PACKAGE', meson.project_name())
conf.set_quoted('PREFIX', get_option('prefix'))
conf.set_quoted('PKGDATADIR', pkgdatadir)
conf.set_quoted('DIALIBDIR',  dialibdir)
conf.set_quoted('LOCALEDIR', get_option('localedir'))
conf.set('ENABLE_NLS', true)

foreach h : ['stddef.h', 'fcntl.h', 'unistd.h', 'utime.h']
    conf.set10('HAVE_' + h.underscorify().to_upper(), cc.has_header(h))
endforeach

foreach f : ['select', 'strcspn', 'strdup', 'strtol']
    conf.set10('HAVE_' + f.underscorify().to_upper(), cc.has_function(f))
endforeach

configuration_inc = include_directories('.')
configure_file(output : 'config.h',
               configuration : conf)

# TODO: I don't think this should be defined.
# To fix we should remove #ifdef HAVE_CONFIG_H checks from all source files.
add_project_arguments('-DHAVE_CONFIG_H', language : 'c')

# Keep track of everything that is needed for running dia
rundeps = []

subdir('po')
subdir('lib')
subdir('objects')
subdir('app')
subdir('data')
subdir('doc')
subdir('plug-ins')
subdir('sheets')
subdir('shapes')
subdir('tests')

# This is a convenience target to create a way to run dia from
# command line.
custom_target('run_with_dia_env',
    command: [find_program('generate_run_with_dia_env.sh'),
              '@OUTPUT@',
              meson.source_root(),
              meson.build_root()
          ],
    output: 'run_with_dia_env',
    # Ensure we always generate this in the build directory.
    build_by_default: true
)

meson.add_install_script('meson-helpers/post-install.py', datadir)
