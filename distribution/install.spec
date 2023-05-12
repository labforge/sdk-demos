# Spec file for packing a Pleora application with its dependencies and Pyside6 select dependencies
# @author Thomas Reidemeister <thomas@labforge.ca>
# @date 2023.03.23
#
#    Copyright 2023 Labforge Inc.
#
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
# -*- mode: python ; coding: utf-8 -*-
from os.path import basename

block_cipher = None


def filter_analysis(a):
    # Remove translations, we are not using these
    new_datas = []
    for key, file_path, file_type in a.datas:
        if key.endswith('.qm'):
            continue
        new_datas.append((key, file_path, file_type))
    a.datas = new_datas

    # Remove not needed DLLs, or system dlls
    blacklist = [## Windows System files
                 'ucrtbase.dll', # Base windows C library, included in Win10

                 ## Visual Studio 2015 runtime files
                 'VCRUNTIME140.dll',
                 'MSVCP140_2.dll',
                 'MSVCP140_1.dll',
                 'MSVCP140.dll',
                 'VCRUNTIME140_1.dll',
                 'api-ms-win-crt-math-l1-1-0.dll',
                 'api-ms-win-crt-heap-l1-1-0.dll',
                 'api-ms-win-crt-locale-l1-1-0.dll',
                 'api-ms-win-crt-runtime-l1-1-0.dll',
                 'api-ms-win-crt-stdio-l1-1-0.dll',
                 'api-ms-win-crt-string-l1-1-0.dll',
                 'api-ms-win-crt-convert-l1-1-0.dll',
                 'api-ms-win-crt-filesystem-l1-1-0.dll',
                 'api-ms-win-crt-environment-l1-1-0.dll',
                 'api-ms-win-crt-process-l1-1-0.dll',
                 'api-ms-win-crt-conio-l1-1-0.dll',
                 'api-ms-win-crt-time-l1-1-0.dll',
                 'api-ms-win-core-profile-l1-1-0.dll',
                 'api-ms-win-core-memory-l1-1-0.dll',
                 'api-ms-win-core-synch-l1-2-0.dll',
                 'api-ms-win-core-libraryloader-l1-1-0.dll',
                 'api-ms-win-core-handle-l1-1-0.dll',
                 'api-ms-win-core-synch-l1-1-0.dll',
                 'api-ms-win-core-sysinfo-l1-1-0.dll',
                 'api-ms-win-core-processthreads-l1-1-1.dll',
                 'api-ms-win-core-file-l2-1-0.dll',
                 'api-ms-win-core-processthreads-l1-1-0.dll',
                 'api-ms-win-core-heap-l1-1-0.dll',
                 'api-ms-win-core-string-l1-1-0.dll',
                 'api-ms-win-core-file-l1-1-0.dll',
                 'api-ms-win-core-namedpipe-l1-1-0.dll',
                 'api-ms-win-core-errorhandling-l1-1-0.dll',
                 'api-ms-win-core-datetime-l1-1-0.dll',
                 'api-ms-win-core-localization-l1-2-0.dll',
                 'api-ms-win-core-util-l1-1-0.dll',
                 'api-ms-win-core-processenvironment-l1-1-0.dll',
                 'api-ms-win-core-rtlsupport-l1-1-0.dll',
                 'api-ms-win-core-file-l1-2-0.dll',
                 'api-ms-win-core-interlocked-l1-1-0.dll',
                 'api-ms-win-core-console-l1-1-0.dll',
                 'api-ms-win-core-debug-l1-1-0.dll',
                 'api-ms-win-core-timezone-l1-1-0.dll',
                 'api-ms-win-crt-utility-l1-1-0.dll',

                 ## QT6 components that are not used
                 # Image formats
                 'qwbmp.dll',
                 'qtiff.dll',
                 'qwebp.dll',
                 'qtga.dll',
                 'qsvgicon.dll',
                 'qgif.dll',
                 'qsvg.dll',
                 'qicns.dll',
                 'qjpeg.dll',
                 'qico.dll',
                 'qpdf.dll',
                 'Qt6Pdf.dll',
                 'Qt6Svg.dll',
                 ## Plugins
                 # Hardware acceleration plugins for display
                 'qdirect2d.dll',
                 'opengl32sw.dll',
                 # Networking
                 'qcertonlybackend.dll',
                 'qopensslbackend.dll',
                 'qschannelbackend.dll',
                 'qnetworklistmanager.dll',
                 ## Platforms
                 # Below needed for minimal GUI
                 #'qwindows.dll',
                 #'qwindowsvistastyle.dll',
                 'qoffscreen.dll',
                 'qtuiotouchplugin.dll',
                 'qminimal.dll',
                 # Input methods
                 'qtvirtualkeyboardplugin.dll',
                 'Qt6VirtualKeyboard.dll',
                 # Not needed qt modules
                 'Qt6Quick.dll',
                 'Qt6Qml.dll',
                 'Qt6QmlModels.dll',
                 'Qt6OpenGL.dll',
                 'Qt6Network.dll',
                 'QtNetwork.pyd',

                 ## Not needed python core libraries
                 '_bz2.pyd',
                 '_hashlib.pyd',
                 '_lzma.pyd',
                 '_ssl.pyd',
                 'libcrypto-1_1.dll',
                 'libssl-1_1.dll',
    ]
    # ignore case
    blacklist = list(map(lambda x: x.lower(), blacklist))
    new_binaries = []
    for key, file_path, file_type in a.binaries:
        dll_filename = basename(key).split('/')[-1]
        if dll_filename.lower() in blacklist:
            continue
        new_binaries.append((key, file_path, file_type))
    a.binaries = new_binaries

    return a


a = Analysis(
    ['../utility/utility.py'],
    pathex=[],
    binaries=[],
    datas=[],
    hiddenimports=['numpy.core.multiarray', 
		'numpy.random.common', 
		'numpy.random.bounded_integers', 
		'numpy.random.entropy',
		'encodings.idna'],
    hookspath=[],
    hooksconfig={},
    runtime_hooks=[],
    excludes=[],
    win_no_prefer_redirects=False,
    win_private_assemblies=False,
    cipher=block_cipher,
    noarchive=False,
)

a = filter_analysis(a)

pyz = PYZ(a.pure, a.zipped_data, cipher=block_cipher)

exe = EXE(
    pyz,
    a.scripts,
    [],
    exclude_binaries=True,
    name='updater',
    debug=False,
    bootloader_ignore_signals=False,
    strip=True,
    upx=True,
    console=False,
    disable_windowed_traceback=False,
    argv_emulation=False,
    target_arch=None,
    codesign_identity=None,
    entitlements_file=None,
    version='version.txt',
    icon=['../utility/img/labforge.ico'],
)
coll = COLLECT(
    exe,
    a.binaries,
    a.zipfiles,
    a.datas,
    strip=False,
    upx=True,
    upx_exclude=[],
    name='main',
)
