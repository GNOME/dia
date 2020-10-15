project('dia',
        ['c', 'cpp'],
        version: '0.97.3',  # This is the base version, git suffix gets added using vcs_tag().
        meson_version: '>= 0.49',
        default_options: ['warning_level=1']
)

cc = meson.get_compiler('c')
conf = configuration_data()

libglib_dep = dependency('glib-2.0',   version: '>= 2.58')
libgtk_dep  = dependency('gtk+-2.0',   version: '>= 2.24.31')
libxml_dep  = dependency('libxml-2.0', version: '>= 2.9.4')
#TODO: what are the minimum versions?
gmodule_dep = dependency('gmodule-2.0')
libzlib_dep = dependency('zlib')
libcairo_dep = dependency('cairo')
graphene_dep = dependency('graphene-1.0')

# Not required since not all platforms ship a separate libm.
libm_dep = cc.find_library('m', required: false)

# This is needed for some platforms (eg. BSD).
libc_dep = cc.find_library('c', required: false)

# Optional deps
libpoppler_dep = dependency('poppler', version: '> 0.62.0', required: false)
conf.set('HAVE_POPPLER', libpoppler_dep.found())
libemf_dep = cc.find_library('emf', required: false)
conf.set('HAVE_LIBEMF', libemf_dep.found())
libogdf_dep = cc.find_library('ogdf', required: false)
conf.set('HAVE_OGDF', libogdf_dep.found())
libxslt_dep = dependency('libxslt', required: false)
conf.set('HAVE_XSLT', libxslt_dep.found())
# TODO: Why does Win32 break without this
conf.set('ENABLE_NLS', true)

libgdk_dep  = dependency('gdk-2.0')
gdk_target = libgdk_dep.get_pkgconfig_variable('target')

if gdk_target == 'quartz'
    gtk_mac_dep = dependency('gtk-mac-integration')
    conf.set('HAVE_MAC_INTEGRATION', true)
else
    # non-dependency
    gtk_mac_dep = dependency('', required: false)
endif

prefix = get_option('prefix')
datadir = prefix / get_option('datadir')

po_dir = meson.current_source_dir() / 'po'
pkgdatadir = datadir / meson.project_name()
dialibdir = prefix / get_option('libdir') / meson.project_name()
dialocaledir = prefix / get_option('localedir')

# Specify a header configuration file
conf.set_quoted('VERSION', '@VCS_TAG@')
conf.set_quoted('GETTEXT_PACKAGE', meson.project_name())
conf.set_quoted('PREFIX', get_option('prefix'))
conf.set_quoted('PKGDATADIR', pkgdatadir)
conf.set_quoted('DIALIBDIR',  dialibdir)
conf.set_quoted('LOCALEDIR', dialocaledir)

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
glib_ver = 'GLIB_VERSION_2_58'
add_project_arguments([
  '-DGDK_VERSION_MIN_REQUIRED=@0@'.format(gtk_ver),
  '-DGDK_VERSION_MAX_ALLOWED=@0@'.format(gtk_ver),
  '-DGLIB_VERSION_MIN_REQUIRED=@0@'.format(glib_ver),
  '-DGLIB_VERSION_MAX_ALLOWED=@0@'.format(glib_ver),
  '-DGSEAL_ENABLE',
  '-DGTK_DISABLE_DEPRECATED',
], language: ['c', 'cpp'])


global_c_args = []
test_c_args = [
  '-Wdeclaration-after-statement',
  ['-Werror=format-security', '-Werror=format=2'],
  '-Werror=missing-declarations',
  '-Wformat-nonliteral',
  '-Wformat-security',
  '-Winit-self',
  '-Wmaybe-uninitialized',
  '-Wno-strict-aliasing',
  '-Wno-unused-parameter',
  '-Wold-style-definition',
  '-Wshadow',
  # When gtk3
  # '-Wstrict-prototypes',
  '-Wswitch-default',
  '-Wswitch-enum',
  '-Wunused-function',
]

foreach arg: test_c_args
  if cc.has_multi_arguments(arg)
    global_c_args += arg
  endif
endforeach
add_project_arguments(
  global_c_args,
  language: 'c'
)


# Follow the file extensions used by GLib for modules
# https://developer.gnome.org/glib/stable/glib-Dynamic-Loading-of-Modules.html#G-MODULE-SUFFIX:CAPS
if host_machine.system() == 'windows'
  g_module_suffix = 'dll'
else
  g_module_suffix = 'so'
endif


subdir('po')
subdir('lib')
subdir('objects')
subdir('app')
subdir('data')
subdir('plug-ins')
subdir('sheets')
subdir('shapes')
if get_option('tests')
    subdir('tests')
endif
if get_option('doc')
    subdir('doc')
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
