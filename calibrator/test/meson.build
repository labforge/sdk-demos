test_device_exe = executable('test_device',
    ['test_device.cc', 'mock_device.cc', 'util.cc',
    '../src/gev/util.cc', '../src/io/util.cc', '../src/gev/util.cc'],
    qt5.preprocess(moc_headers: 'test_device.hpp', include_directories: app_inc),
    include_directories : app_inc,
    dependencies : [
      app_deps,
    ],
    cpp_args: test_cpp_args,
    link_args: test_link_args
)

test_parameters_exe = executable('test_parameters',
    ['test_parameters.cc', 'mock_device.cc', 'util.cc',
     '../src/ui/parameterwidget.cc'],
    qt5.preprocess(moc_headers: ['test_parameters.hpp', '../inc/ui/parameterwidget.hpp'],
        include_directories: app_inc),
    include_directories : app_inc,
    dependencies : [
      app_deps,
    ],
    cpp_args: test_cpp_args,
    link_args: test_link_args
)

test_pipeline_exe = executable('test_pipeline',
    ['test_pipeline.cc', 'mock_device.cc', 'util.cc',
    '../src/gev/pipeline.cc', '../src/gev/util.cc', '../src/io/util.cc'],
    qt5.preprocess(moc_headers: [
        'test_pipeline.hpp',
        '../inc/gev/pipeline.hpp'],
        include_directories: app_inc),
    include_directories : app_inc,
    dependencies : [
      app_deps,
    ],
    cpp_args: test_cpp_args,
    link_args: test_link_args
)

test_io_util = executable('test_io_util',
    ['test_io_util.cc',
     '../src/io/util.cc'],
    qt5.preprocess(moc_headers: ['test_io_util.hpp'],
        include_directories: app_inc),
    include_directories : app_inc,
    dependencies : [
      app_deps,
    ],
    cpp_args: test_cpp_args,
    link_args: test_link_args
)

test('test_device',
    test_device_exe,
    args : ['-nocrashhandler'],
    workdir : meson.current_source_dir(), # allows local paths to be used from this folder
    timeout : 1000,
    is_parallel : false)

test('test_parameters',
    test_parameters_exe,
    workdir : meson.current_source_dir(), # allows local paths to be used from this folder
    timeout : 1000,
    is_parallel : false)

test('test_io_util',
    test_io_util,
    workdir : meson.current_source_dir(), # allows local paths to be used from this folder
    timeout : 1000,
    is_parallel : false)

test('test_pipeline',
    test_pipeline_exe,
    workdir : meson.current_source_dir(), # allows local paths to be used from this folder
    timeout : 1000,
    is_parallel : false)

mock_device = executable('mock_device',
    ['device.cc', 'mock_device.cc', 'util.cc'],
    include_directories : app_inc,
    dependencies : [
      app_deps,
    ],
    cpp_args: test_cpp_args,
    link_args: test_link_args,
    install: false
)


