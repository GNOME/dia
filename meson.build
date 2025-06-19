project('dia',
        ['c', 'cpp'],
        version: '0.98.0',  # This is the base version, git suffix gets added using vcs_tag().
        meson_version: '>= 1.1.0',
        default_options: [
                             'warning_level=1',
                             'buildtype=debugoptimized',
                             'c_std=gnu17',
                             'cpp_std=gnu++20',
                         ],
)

cc = meson.get_compiler('c')
conf = configuration_data()
conf.set_quoted('DIA_APPLICATION_PATH', '/org/gnome/Dia/')

libglib_dep = dependency('glib-2.0',   version: '>= 2.76')
gio_dep = dependency('gio-2.0',   version: '>= 2.76')
libgtk_dep  = dependency('gtk+-3.0',   version: '>= 3.24')
libxml_dep  = dependency('libxml-2.0', version: '>= 2.14.0')
#TODO: what are the minimum versions?
gmodule_dep = dependency('gmodule-2.0')
libcairo_dep = dependency('cairo')
graphene_dep = dependency('graphene-1.0', version: '>= 1.10')
xpm_dep = dependency('xpm-pixbuf')

# Not required since not all platforms ship a separate libm.
libm_dep = cc.find_library('m', required: false)

# This is needed for some platforms (eg. BSD).
libc_dep = cc.find_library('c', required: false)

# This is needed for generating the man pages
doc_feature = get_option('doc')
xsltproc = find_program('xsltproc', required: false)
doc_feature = doc_feature.disable_if(
    not xsltproc.found(),
    error_message: 'xsltproc is required for doc targets'
)
if xsltproc.found()
    stylesheet = 'http://docbook.sourceforge.net/release/xsl/current/xhtml/chunk.xsl'
    testrun = run_command(xsltproc, '--nonet', stylesheet, check: false)
    doc_feature = doc_feature.disable_if(
        testrun.returncode() != 0,
        error_message: 'stylesheet for generating man pages not found. install docbook-xsl or a similar package.'
    )
endif

# Optional deps
libpoppler_dep = dependency('poppler', version: '> 21.03.0', required: false)
libpopplercpp_dep = dependency('poppler-cpp', required: false)
conf.set('HAVE_POPPLER', libpoppler_dep.found() and libpopplercpp_dep.found())
libemf_dep = cc.find_library('EMF', required: false)
conf.set('HAVE_LIBEMF', libemf_dep.found())
libogdf_dep = cc.find_library('ogdf', required: false)
conf.set('HAVE_OGDF', libogdf_dep.found())
libxslt_dep = dependency('libxslt', required: false)
conf.set('HAVE_XSLT', libxslt_dep.found())
# TODO: Why does Win32 break without this
conf.set('ENABLE_NLS', true)


prefix = get_option('prefix')
datadir = prefix / get_option('datadir')

po_dir = meson.current_source_dir() / 'po'
pkgdatadir = datadir / meson.project_name()
dialibdir = prefix / get_option('libdir') / meson.project_name()
dialocaledir = prefix / get_option('localedir')

# Specify a header configuration file
conf.set_quoted('GETTEXT_PACKAGE', meson.project_name())
conf.set_quoted('PREFIX', get_option('prefix'))
conf.set_quoted('PKGDATADIR', pkgdatadir)
conf.set_quoted('DIALIBDIR',  dialibdir)
conf.set_quoted('LOCALEDIR', dialocaledir)

foreach h : ['unistd.h', 'utime.h']
    conf.set10('HAVE_' + h.underscorify().to_upper(), cc.has_header(h))
endforeach

config_inc = include_directories('.')
config_h = configure_file(output: 'config.h', configuration: conf)
config_inc = include_directories('.')
config_dep = declare_dependency(
    sources: [config_h],
    include_directories: config_inc,
)

gtk_ver = 'GDK_VERSION_3_8'
glib_ver = 'GLIB_VERSION_2_76'
add_project_arguments([
  '-DGDK_VERSION_MIN_REQUIRED=@0@'.format(gtk_ver),
#  '-DGDK_VERSION_MAX_ALLOWED=@0@'.format(gtk_ver),
  '-DGLIB_VERSION_MIN_REQUIRED=@0@'.format(glib_ver),
  '-DGLIB_VERSION_MAX_ALLOWED=@0@'.format(glib_ver),
  '-DGSEAL_ENABLE',
#  '-DGTK_DISABLE_DEPRECATED',
], language: ['c', 'cpp'])


global_c_args = []
test_c_args = [
  '-Wdeclaration-after-statement',
  ['-Werror=format-security', '-Werror=format=2'],
  '-Werror=incompatible-pointer-types',
  '-Werror=missing-declarations',
  '-Werror=strict-prototypes',
  '-Wformat-nonliteral',
  '-Wformat-security',
  '-Winit-self',
  '-Wmaybe-uninitialized',
  '-Wno-strict-aliasing',
  '-Wno-unused-parameter',
  '-Wold-style-definition',
  '-Wshadow',
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

dia_source = meson.project_source_root()
dia_build = meson.project_build_root()

# Setup local environment to run dia.
run_env_dict = {
    'DIA_BASE_PATH'  : [dia_source],
    'DIA_LIB_PATH'   : [dia_build / 'objects', dia_build / 'plug-ins'],
    'DIA_SHAPE_PATH' : [dia_source / 'shapes'],
    'DIA_XSLT_PATH'  : [dia_source / 'plug-ins' / 'xslt'],
    'DIA_PYTHON_PATH': [dia_source / 'plug-ins' / 'python'],
    'DIA_SHEET_PATH' : [dia_build / 'sheets'],
}

run_env = environment()
foreach k, v : run_env_dict
    run_env.set(k, v)
endforeach

# Otherwise Windows won't be able to find libdia.dll in build directory.
if host_machine.system() == 'windows'
    run_env.append('PATH', dia_build / 'lib')
endif

meson.add_devenv(run_env)

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
if doc_feature.allowed()
    subdir('doc')
endif
subdir('docs')

gnome.post_install(
    gtk_update_icon_cache: true,
    update_desktop_database: true
)

if build_machine.system() == 'windows' and get_option('buildtype') == 'release'
    meson.add_install_script('build-aux/windows-bundler.sh')
endif
