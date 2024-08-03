project('sbox', 'cpp', default_options: [
    'default_library=static',
    'c_std=c2x',
    'cpp_std=c++2a'
], subproject_dir: 'external')

src = []
dep = []
inc = []
arg = []

inc += include_directories('src')

dep += [
  dependency('spdlog'),
  dependency('glm'),
]

cmake = import('cmake')
vkbootstrap = cmake.subproject('vk-bootstrap')
dep += vkbootstrap.dependency('vk-bootstrap')
glfw = cmake.subproject('glfw')
dep += glfw.dependency('glfw')

inc += include_directories('external')
inc += include_directories('external/vk-bootstrap/src')

subdir('src')

executable(
  'sbox',
  src,
  dependencies : dep,
  include_directories : inc,
  cpp_args : arg,
  install : false
)
