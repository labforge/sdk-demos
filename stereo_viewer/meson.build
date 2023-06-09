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
    meson_version : '>= 0.60.0',
    default_options : ['cpp_std=c++20',
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
qt5_deps = dependency('qt5', modules: ['Core', 'Gui', 'Widgets', 'Test', 'Network'])
opencv_dep = dependency('opencv4', required: false)
if not opencv_dep.found()
  # Windows hacks, version info is appened to cvlibs for cl compiler, but not in cmake specs, so hardcode libs here
  cv4_core = cpp.find_library('opencv_core430', dirs: ['C:\cv4\x64\vc15\bin', 'C:\cv4\lib'])
  cv4_gui = cpp.find_library('opencv_highgui430', dirs: ['C:\cv4\x64\vc15\bin', 'C:\cv4\lib'])
  cv4_codec = cpp.find_library('opencv_imgcodecs430', dirs: ['C:\cv4\x64\vc15\bin', 'C:\cv4\lib'])
  cv4_imgproc = cpp.find_library('opencv_imgproc430', dirs: ['C:\cv4\x64\vc15\bin', 'C:\cv4\lib'])
  opencv_dep = declare_dependency(version: '4.3.0',
                                  compile_args: ['-IC:/cv4/include'],
                                  dependencies : [cv4_core, cv4_gui, cv4_codec, cv4_imgproc],
                                  )
endif
if host_machine.system() == 'linux'
  pleora_dirs = ['/opt/pleora/ebus_sdk/Ubuntu-18.04-x86_64/lib',
                 '/opt/pleora/ebus_sdk/Ubuntu-20.04-x86_64/lib',
                 '/opt/pleora/ebus_sdk/Ubuntu-22.04-x86_64/lib/']
  pleora_inc = ['-I/opt/pleora/ebus_sdk/Ubuntu-18.04-x86_64/include',
                '-I/opt/pleora/ebus_sdk/Ubuntu-20.04-x86_64/include/',
                '-I/opt/pleora/ebus_sdk/Ubuntu-22.04-x86_64/include/']
  pleora_args = pleora_inc+'-DQT_GUI_LIB'
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
  pleora_args = [pleora_inc, '-D_AFXDLL']
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
                                                PvPersistence
                                                ],
                                compile_args: pleora_args
                               )

# Lump together all monolithic dependencies from above
app_deps = [
  qt5_deps,
  opencv_dep,
  pleora_dep,
]

subdir('src')
