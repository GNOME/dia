project('dia',
        ['c', 'cpp'],
        version: '0.97.3',  # This is the base version, git suffix gets added using vcs_tag().
        meson_version: '>= 0.49',
        default_options: []
)

cc = meson.get_compiler('c')
conf = configuration_data()

libglib_dep = dependency('glib-2.0',   version: '>= 2.50.3')
libgtk_dep  = dependency('gtk+-2.0',   version: '>= 2.24.31')
libxml_dep  = dependency('libxml-2.0', version: '>= 2.9.4')
#TODO: what are the minimum versions?
gmodule_dep = dependency('gmodule-2.0')
libzlib_dep = dependency('zlib')
libcairo_dep = dependency('cairo')
#TODO: this will become a hard dependency as part of GTK3 port.
# Therefore, remove all references to HAVE_CAIRO
conf.set('HAVE_CAIRO', 1)

# Not required since not all platforms ship a separate libm.
libm_dep = cc.find_library('m', required: false)

# This is needed for some platforms (eg. BSD).
libc_dep = cc.find_library('c', required: false)

# Optional deps
# TODO: Until issue #23 is fixed, we need to keep version below 0.62.
libpoppler_dep = dependency('poppler', version: '<= 0.62.0', required: false)
conf.set('HAVE_POPPLER', libpoppler_dep.found())
libemf_dep = cc.find_library('emf', required: false)
conf.set('HAVE_LIBEMF', libemf_dep.found())
libogdf_dep = cc.find_library('ogdf', required: false)
conf.set('HAVE_OGDF', libogdf_dep.found())
libxslt_dep = dependency('libxslt', required: false)
conf.set('HAVE_XSLT', libxslt_dep.found())

datadir = join_paths(get_option('prefix'),
                     get_option('datadir'))

pkgdatadir = join_paths(datadir, meson.project_name())

dialibdir = join_paths(get_option('prefix'),
                       get_option('libdir'),
                       meson.project_name())

# Specify a header configuration file
conf.set_quoted('VERSION', '@VCS_TAG@')
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
config_h_in = configure_file(output: 'config.h.in', configuration: conf)
config_h = vcs_tag(input: config_h_in, output: 'config.h')

gtk_ver = 'GDK_VERSION_2_24'
glib_ver = 'GLIB_VERSION_2_50'
add_project_arguments([
  '-DGDK_VERSION_MIN_REQUIRED=@0@'.format(gtk_ver),
  '-DGDK_VERSION_MAX_ALLOWED=@0@'.format(gtk_ver),
  '-DGLIB_VERSION_MIN_REQUIRED=@0@'.format(glib_ver),
  '-DGLIB_VERSION_MAX_ALLOWED=@0@'.format(glib_ver),
], language: 'c')

subdir('po')
subdir('lib')
subdir('objects')
subdir('app')
subdir('data')
subdir('doc')
subdir('plug-ins')
subdir('sheets')
subdir('shapes')
if get_option('tests')
    subdir('tests')
endif
subdir('docs')

# TODO: idealy this should use run_env from tests/
# Unfortunately, neither run_target or custom_target support env
# https://github.com/mesonbuild/meson/issues/2723
# Alternatively we could use run configurations:
# https://github.com/mesonbuild/meson/issues/1314
custom_target('run_with_dia_env',
    command: [find_program('generate_run_with_dia_env.sh'),
              '@OUTPUT@',
              meson.source_root(),
              meson.build_root()
          ],
    output: 'run_with_dia_env',
    # Ensure we always generate this in the build directory.
    build_by_default: true,
)

meson.add_install_script('build-aux/post-install.py', datadir)

if build_machine.system() == 'windows' and get_option('buildtype') == 'release'
    meson.add_install_script('build-aux/windows-bundler.sh')
endif
