qt_app_headers = [
  '../inc/ui/MainWindow.hpp',
  '../inc/ui/cameraview.hpp',
  #'../inc/ui/parameterwidget.hpp',
  '../inc/gev/pipeline.hpp',
  '../inc/io/data_thread.hpp',
  ]

qtprocessed = qt5.preprocess(moc_headers: qt_app_headers,
                             ui_files: '../calibrator.ui',
                             include_directories: app_inc)

source_files = files([
  'calibrator.cc',
  'io/util.cc',
  'gev/pipeline.cc',
  'gev/util.cc',
  'ui/cameraview.cc',
  'io/data_thread.cc',
])

if host_machine.system() == 'linux'
  stereo_aq = executable(
    'calibrator',
    source_files,
    qtprocessed,
    git_hdr,
    include_directories : app_inc,
    install: true,
    dependencies: [app_deps],
    cpp_args : ['-Wno-deprecated', '-Wno-deprecated-copy'],
  )
else
  stereo_aq = executable(
    'calibrator',
    source_files,
    qtprocessed,
    git_hdr,
    include_directories : app_inc,
    install: true,
    dependencies: [app_deps],
    gui_app: true,
    link_args : ['/SUBSYSTEM:WINDOWS', 'C:/qt5/lib/qtmain.lib']
  )
endif
