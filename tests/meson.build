# TODO: minimise code duplication.
bb = executable(
    'boundingbox',
    'test-boundingbox.c',
    dependencies: [libgtk_dep, libxml_dep, libm_dep],
    link_with: [libdia],
    include_directories: [configuration_inc, libdia_inc],
)
objs = executable(
    'objects',
    'test-objects.c',
    dependencies: [libgtk_dep, libxml_dep, libm_dep],
    link_with: [libdia],
    include_directories: [configuration_inc, libdia_inc],
)
tsvg = executable(
    'testsvg',
    'test-svg.c',
    dependencies: [libgtk_dep, libxml_dep, libm_dep],
    link_with: [libdia],
    include_directories: [configuration_inc, libdia_inc],
)
test('boundingbox', bb)
test('objects', objs, args: [join_paths(meson.build_root(), 'objects')])
test('testsvg', tsvg)
