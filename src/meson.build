videoplayer_sources = [
  'player/VideoPlayer.cpp',
  'player/VideoDecoder.cpp',
  'main.cpp',
]

videoplayer_deps = [
  dependency('libavformat', method: 'pkg-config'),
  dependency('libavcodec', method: 'pkg-config'),
  dependency('libavutil', method: 'pkg-config'),
  dependency('libswscale', method: 'pkg-config'),

  dependency('glfw3'),
  dependency('gl'),
]


executable('videoplayer', videoplayer_sources, glad_sources,
  dependencies: videoplayer_deps,
  include_directories: glad_includes,
  install: true,
)

