sources = files(
    'diacairo.c',
    'diacairo-interactive.c',
    'diacairo-renderer.c',
    'diacairo-print.c'
)

install_plugins_desc += {
    'name': 'cairo_filter',
    'sources': sources,
    'extra_deps': [libcairo_dep],
}
