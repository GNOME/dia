icons = [ 'arc.png',
          'beziergon.png',
          'bezierline.png',
          'box.png',
          'ellipse.png',
          'image.png',
          'line.png',
          'outline.png',
          'polygon.png',
          'polyline.png',
          'text.png',
          'zigzagline.png' ]

#TODO: this is duplicated from lib/pixmaps.  Any way to remove this duplication?
icon_pairs = []
foreach icon_file : icons
    icon_pairs += [icon_file.split('.')[0].underscorify() + '_icon', files(icon_file)]
endforeach

tool_icons_h = custom_target(
    'tool-icons.h',
    output : 'tool-icons.h',
    capture : true,
    command : [ gdk_pixbuf_csource,
                '--raw',
                '--build-list',
                icon_pairs ])
