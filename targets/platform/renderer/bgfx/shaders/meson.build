shaderc = find_program('shaderc')

include_dir = join_paths(
  meson.global_source_root(),
  'subprojects', 'bgfx', 'src'
)

output_dir = join_paths(
  meson.source_root()
)

shader_targets = {
  '440': '_glsl',
  'spirv': '_spv',
}

if host_machine.system() == 'darwin'
  shader_targets += {'metal': '_mtl'}
endif

vs_headers = []
fs_headers = []

foreach profile, suffix : shader_targets

  fs_headers += custom_target(
    'fs_4jcraft_' + profile,
    input: 'fs_4jcraft.sc',
    output: 'fs_4jcraft' + suffix + '.bin.h',
    command: [
      shaderc,
      '-f', '@INPUT@',
      '-o', '@OUTPUT@',
      '--bin2c',
      '-i', include_dir,
      '--platform', host_machine.system(),
      '--profile', profile,
      '--type', 'f',
    ],
    build_always: true,
    install: true,
    install_dir: '.'
  )

  vs_headers += custom_target(
    'vs_4jcraft_' + profile,
    input: 'vs_4jcraft.sc',
    output: 'vs_4jcraft' + suffix + '.bin.h',
    command: [
      shaderc,
      '-f', '@INPUT@',
      '-o', '@OUTPUT@',
      '--bin2c',
      '-i', include_dir,
      '--platform', host_machine.system(),
      '--profile', profile,
      '--type', 'v',
    ],
    build_always: true,
    install: true,
    install_dir: '.'
  )

endforeach

vs_shader = custom_target(
    'vs_4jcraft',
    output: 'vs_4jcraft.bin.h',
    input: vs_headers,
    command: [
    'sh', '-c',
    'cat "$@" > "$1"',
    'sh', '@OUTPUT@', '@INPUT@',
    ],
    install_dir: '.',
    install: true,
    build_by_default: true,
)

fs_shader = custom_target(
    'fs_4jcraft',
    output: 'fs_4jcraft.bin.h',
    input: fs_headers,
    command: [
    'sh', '-c',
    'cat "$@" > "$1"',
    'sh', '@OUTPUT@', '@INPUT@',
    ],
    install_dir: '.',
    install: true,
    build_by_default: true,
)

shader_dep = declare_dependency(
  include_directories: '.'
)

meson.override_dependency('shaders',shader_dep)
