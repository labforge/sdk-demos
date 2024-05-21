## Project definition
#  Copyright (C) 2013-2023 Labforge Inc.
#  Author: Thomas Reidemeister <thomas@labforge.ca>
#    Licensed under the Apache License, Version 2.0 (the "License");
#    you may not use this file except in compliance with the License.
#    You may obtain a copy of the License at
#
#        http://www.apache.org/licenses/LICENSE-2.0
#
#    Unless required by applicable law or agreed to in writing, software
#    distributed under the License is distributed on an "AS IS" BASIS,
#    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#    See the License for the specific language governing permissions and
#    limitations under the License.
project('Bottlenose Stereo Viewer', 'cpp',
    version : '0.1.100',
    license : 'Labforge',
    meson_version : '>= 0.52.0',
    default_options : ['cpp_std=c++17',
                       'warning_level=3',
                       'buildtype=release',
                       'b_ndebug=if-release',
                       ],
    )

app_inc = include_directories(
	'.',
	'inc',
	)
## Add all library dependencies here before subdirectories and executables
cpp = meson.get_compiler('cpp')
# set PATH=C:\qt5\bin;%PATH%
qt5 = import('qt5')
qt5_deps = dependency('qt5', modules: ['Core', 'Gui', 'Widgets', 'Test', 'Network', 'Concurrent'], main: true)

opencv_dep = dependency('opencv4', required: false)
if not opencv_dep.found()
  cv_dir = get_option('cv_dir')
  cv4_lib_dirs = [
    join_paths(cv_dir, 'x64', 'vc14', 'bin'),
    join_paths(cv_dir, 'x64', 'vc15', 'bin'),
    join_paths(cv_dir, 'lib')
  ]
  message(cv4_lib_dirs)
  cv4_include_path = join_paths(cv_dir, 'include')
  # Precompiled
  cv4_core = cpp.find_library('opencv_core430', dirs: cv4_lib_dirs)
  cv4_gui = cpp.find_library('opencv_highgui430', dirs: cv4_lib_dirs)
  cv4_codec = cpp.find_library('opencv_imgcodecs430', dirs: cv4_lib_dirs)
  cv4_imgproc = cpp.find_library('opencv_imgproc430', dirs: cv4_lib_dirs)
  cv4_calib = cpp.find_library('opencv_calib3d430', dirs: cv4_lib_dirs)
  cv4_feat = cpp.find_library('opencv_features2d430', dirs: cv4_lib_dirs)
  cv4_flann = cpp.find_library('opencv_flann430', dirs: cv4_lib_dirs)

  opencv_dep = declare_dependency(version: '4.3.0',
                                  compile_args: ['-I' + cv4_include_path],
                                  dependencies : [cv4_core, cv4_gui, cv4_codec, cv4_imgproc, cv4_calib, cv4_feat, cv4_flann],
                                  )
endif
if host_machine.system() == 'linux'
  pleora_dirs = ['/opt/pleora/ebus_sdk/Ubuntu-18.04-x86_64/lib',
                 '/opt/pleora/ebus_sdk/Ubuntu-20.04-x86_64/lib',
                 '/opt/pleora/ebus_sdk/Ubuntu-22.04-x86_64/lib/']
  pleora_inc = ['-I/opt/pleora/ebus_sdk/Ubuntu-18.04-x86_64/include',
                '-I/opt/pleora/ebus_sdk/Ubuntu-20.04-x86_64/include/',
                '-I/opt/pleora/ebus_sdk/Ubuntu-22.04-x86_64/include/']
  pleora_args = pleora_inc+['-DQT_GUI_LIB']
  PvBase = cpp.find_library('PvBase', required: true, dirs: pleora_dirs)
  PvBuffer = cpp.find_library('PvBuffer', required: true, dirs: pleora_dirs)
  PvSystem = cpp.find_library('PvSystem', required: true, dirs: pleora_dirs)
  PvDevice = cpp.find_library('PvDevice', required: true, dirs: pleora_dirs)
  PvGenICam = cpp.find_library('PvGenICam', required: true, dirs: pleora_dirs)
  PvStream = cpp.find_library('PvStream', required: true, dirs: pleora_dirs)
  PvVirualDevice = cpp.find_library('PvVirtualDevice', required: true, dirs: pleora_dirs)
  PvGUI = cpp.find_library('PvGUI', required: true, dirs: pleora_dirs)
  PvAppUtils = cpp.find_library('PvAppUtils', required: true, dirs: pleora_dirs)
  PvPersistence = cpp.find_library('PvPersistence', required: true, dirs: pleora_dirs)
else # Windows, note force 64-bit
  pleora_dirs = ['C:/Program Files/Pleora Technologies Inc/eBUS SDK/Libraries']
  pleora_inc = '-IC:/Program Files/Pleora Technologies Inc/eBUS SDK/Includes'
  pleora_args = [pleora_inc, '-D_AFXDLL', '/MD', '-D_CRT_SECURE_NO_WARNINGS']
  PvBase = cpp.find_library('PvBase64', required: true, dirs: pleora_dirs)
  PvBuffer = cpp.find_library('PvBuffer64', required: true, dirs: pleora_dirs)
  PvSystem = cpp.find_library('PvSystem64', required: true, dirs: pleora_dirs)
  PvDevice = cpp.find_library('PvDevice64', required: true, dirs: pleora_dirs)
  PvGenICam = cpp.find_library('PvGenICam64', required: true, dirs: pleora_dirs)
  PvStream = cpp.find_library('PvStream64', required: true, dirs: pleora_dirs)
  PvVirualDevice = cpp.find_library('PvVirtualDevice64', required: true, dirs: pleora_dirs)
  PvGUI = cpp.find_library('PvGUI64_VC16', required: true, dirs: pleora_dirs)
  PvAppUtils = cpp.find_library('PvAppUtils64', required: true, dirs: pleora_dirs)
  PvPersistence = cpp.find_library('PvPersistence64', required: true, dirs: pleora_dirs)
endif
pleora_dep = declare_dependency(compile_args: pleora_inc,
                                dependencies : [PvBase,
                                                PvBuffer,
                                                PvSystem,
                                                PvDevice,
                                                PvGenICam,
                                                PvStream,
                                                PvGUI,
                                                PvVirualDevice,
                                                PvAppUtils,
                                                PvPersistence,
                                                ],
                                compile_args: pleora_args
                               )

# json
nlohmann_json_dep = dependency('nlohmann_json', fallback : ['nlohmann_json', 'nlohmann_json_dep'])

# yaml-cppnau
yamlcpp_dep = dependency('yaml-cpp', version: '>= 0.6.1', required: false)
if not yamlcpp_dep.found()
  yamlcmake = import('cmake')
  shared = 'OFF'
  if host_machine.system() == 'windows'
    shared = 'ON'
  endif
  yamlcpp_dep = yamlcmake.subproject('yaml-cpp', cmake_options: [
      '-DCMAKE_BUILD_TYPE=Release',
      '-DCMAKE_POSITION_INDEPENDENT_CODE=ON',
      '-DYAML_CPP_BUILD_TOOLS=OFF',
      '-DYAML_CPP_BUILD_TESTS=OFF',
      '-DYAML_CPP_BUILD_CONTRIB=OFF',
      '-DBUILD_SHARED_LIBS=' + shared
  ]).dependency('yaml-cpp')
endif

# Lump together all monolithic dependencies from above
app_deps = [
  qt5_deps,
  opencv_dep,
  pleora_dep,
  nlohmann_json_dep,
  yamlcpp_dep
]

subdir('src')
