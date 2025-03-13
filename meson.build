project('fractal_explorer', ['c', 'cpp'] )
wayland_client = dependency('wayland-client')
wayland_protos = dependency('wayland-protocols')
wayland_scanner = dependency('wayland-scanner')


wl_protocol_dir = wayland_protos.get_variable('pkgdatadir')

wayland_scanner_prog = find_program(wayland_scanner.get_variable('wayland_scanner'), native: true)

wayland_scanner_code = generator(
  wayland_scanner_prog,
  output: '@BASENAME@-protocol.c',
  arguments: ['private-code', '@INPUT@', '@OUTPUT@'],
)

wayland_scanner_client = generator(
  wayland_scanner_prog,
  output: '@BASENAME@-client-protocol.h',
  arguments: ['client-header', '@INPUT@', '@OUTPUT@'],
)

protos_src = []

client_protocols = [
  wl_protocol_dir / 'stable/xdg-shell/xdg-shell.xml',
]

foreach filename : client_protocols
  protos_src += wayland_scanner_code.process(filename)
  protos_src += wayland_scanner_client.process(filename)
endforeach

sources = [
    'main.cpp',
    'window.cpp',
    'extramath.cpp',
    'fractal_explorer.cpp',
    protos_src
]
executable('fex', sources, include_directories: [ './' ], dependencies: [ wayland_client ], install: true,)
