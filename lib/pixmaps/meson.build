# Generate csource for the icons.
icons =  [ 'broken.png', 'broken-chain.png', 'unbroken-chain.png', 'standard-path.png' ]

#TODO: this is duplicated in app/pixmaps, lib/pixmaps and objects/standard/pixmaps.
# Remove from all three once we port from gdk_pixbuf_csource to GResource.
icon_pairs = []
foreach icon_file : icons
    icon_pairs += ['dia_' + icon_file.split('.')[0].underscorify() + '_icon', files(icon_file)]
endforeach

dia_lib_icons_h = custom_target(
    'dia-lib-icons.h',
    output : 'dia-lib-icons.h',
    capture : true,
    command : [ gdk_pixbuf_csource,
                '--build-list',
                icon_pairs])
