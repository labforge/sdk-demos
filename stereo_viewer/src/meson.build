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
qt_app_headers = [
  '../inc/ui/MainWindow.hpp',
  '../inc/ui/cameraview.hpp',
  '../inc/gev/pipeline.hpp',
  '../inc/io/data_thread.hpp',
  ]

qtprocessed = qt5.preprocess(moc_headers: qt_app_headers,
                             ui_files: '../stereo_viewer.ui',
                             qresources: '../resources.qrc',
                             include_directories: app_inc)

source_files = files([
  'viewer.cc',
  'focus.cc',
  'bottlenose_chunk_parser.cc',
  'io/util.cc',
  'gev/pipeline.cc',
  'gev/util.cc',
  'ui/cameraview.cc',
  'io/data_thread.cc',
  'gev/stereo_rig.cc',
])

if host_machine.system() == 'linux'
  stereo_aq = executable(
    'stereoviewer',
    source_files,
    qtprocessed,
    include_directories : app_inc,
    install: true,
    dependencies: [app_deps],
    cpp_args : ['-Wno-deprecated', '-Wno-deprecated-copy'],
  )
else
  # Compile rc resources as well
  windows = import('windows')
  windows_rc = windows.compile_resources('../stereo_viewer.rc')
  stereo_aq = executable(
    'stereoviewer',
    source_files,
    qtprocessed,
    windows_rc,
    include_directories : app_inc,
    install: true,
    dependencies: [app_deps],
    gui_app: true,
    link_args : ['/SUBSYSTEM:WINDOWS', 'C:/qt5/lib/qtmain.lib']
  )
endif
