icons = [ 'connectable.png',
          'connectable-empty.png',
          'selectable.png',
          'selectable-empty.png',
          'on-grid.png',
          'off-grid.png',
          'visible.png',
          'visible-empty.png',
          'group.png',
          'ungroup.png',
          'mainpoints-on.png',
          'mainpoints-off.png',


          'objects-layer-above.png',
          'objects-layer-below.png',
          'layers.png',
          'layer-add.png',
          'layer-rename.png',

          'cursor-create.png',
          'cursor-hand-open.png',
          'cursor-hand-closed.png',
          'cursor-magnify-minus.png',
          'cursor-magnify-plus.png' ]

#FIXME: this is duplicated from lib/pixmaps.  Any way to remove this duplication?
icon_pairs = []
foreach icon_file : icons
    icon_pairs += ['dia_' + icon_file.split('.')[0].underscorify() + '_icon', files(icon_file)]
endforeach

# Add a few custom ones.
icon_pairs += [
    'dia_modify_tool_icon', files('arrow-22.png'),
    'dia_scroll_tool_icon', files('scroll-22.png'),
    'dia_textedit_tool_icon', files('textedit-22.png'),
    'dia_zoom_tool_icon', files('zoom-22.png'),
    'dia_diagram_icon', files('dia-diagram.png'),
    'dia_app_icon', files('dia-app.png')
]

dia_app_icons_h = custom_target(
    'dia-app-icons.h',
    output : 'dia-app-icons.h',
    capture : true,
    command : [ gdk_pixbuf_csource,
                '--raw',
                '--build-list',
                icon_pairs ])
