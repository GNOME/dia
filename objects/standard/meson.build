subdir('pixmaps')

object_sources = [
    'arc.c',
    'box.c',
    'ellipse.c',
    'textobj.c',
    'line.c',
    'zigzagline.c',
    'polyline.c',
    'bezier.c',
    'standard.c',
    'image.c',
    'outline.c',
    'polygon.c',
    'beziergon.c'
]

library('standard_objects',
    object_sources + [tool_icons_h],
    dependencies : [libgtk_dep, libglib_dep, libxml_dep, libm_dep],
    link_with : libdia,
    include_directories : [configuration_inc, libdia_inc],
    install : true,
    install_dir : dialibdir
)

