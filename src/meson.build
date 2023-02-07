videoplayer_sources = [
  'player/VideoPlayer.cpp',
  'player/VideoDecoder.cpp',
  'main.cpp',
]

videoplayer_deps = [
  dependency('libavformat'),
  dependency('libavcodec'),
  dependency('libavutil'),
  dependency('libswscale'),

  dependency('glfw3'),
  dependency('opengl'),
  dependency('glm')

]

add_project_arguments(['-ldl'], language: 'cpp')

executable('videoplayer', videoplayer_sources, glad_sources,
  dependencies: videoplayer_deps,
  include_directories: glad_includes,
  install: true,
)

