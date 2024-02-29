::=============================================================================
:: Script: env.bat
:: Author: Thomas Reidemeister <thomas@labforge.ca>
::   This script bootstraps the paths and needed tools for Windows builds/development.
::
::   Requirements:
::     Powershell
::     Python > 3.7 (and in path)
::     MS Build tools configured for Visual C++ (Desktop), https://aka.ms/vs/17/release/vs_buildtools.exe
::
::   Installations of the Script
::     Visual C++ 2022 Build Tools -> %ProgramFiles(x86)%\Microsoft Visual Studio\
::     QT5 5.1.12 LGPL components  -> %ProgramFiles\qt5
::     OpenCV 4.3.0                -> %ProgramFiles\cv4
::     XXD                         -> C:\Windows\System32\
::
:: Copyright (C) 2013-2023 Labforge Inc.
::
::    Licensed under the Apache License, Version 2.0 (the "License");
::    you may not use this file except in compliance with the License.
::    You may obtain a copy of the License at
::
::        http://www.apache.org/licenses/LICENSE-2.0
::
::    Unless required by applicable law or agreed to in writing, software
::    distributed under the License is distributed on an "AS IS" BASIS,
::    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
::    See the License for the specific language governing permissions and
::    limitations under the License.
::
::=============================================================================
@ECHO OFF
:: VS Build tools
IF NOT EXIST "%ProgramFiles(x86)%\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" (
    ECHO Please install VC Build tools for Visual C++
    EXIT /B 1
) ELSE (
    ECHO VC Build tools found
)
:: Configure environment
CALL "%ProgramFiles(x86)%\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" x64

:: QT5
IF NOT DEFINED QTHOME (
    SET QTHOME=C:\qt5
)
IF NOT EXIST %QTHOME% (
    ECHO QT5 not properly installed ... exiting
    EXIT /B 1
) ELSE (
    SET "PATH=%QTHOME%\bin;%QTHOME%\lib;%PATH%"
    ECHO QT5 configured
)

:: OpenCV4
IF NOT DEFINED OPENCVHOME (
    SET OPENCVHOME=C:\cv4
)
IF NOT EXIST %OPENCVHOME% (
    ECHO OpenCV not properly installed ... exiting
    EXIT /B 1
) ELSE (
    SET "PATH=%OPENCVHOME%\bin;%OPENCVHOME%\lib;%PATH%"
)

WHERE xxd
IF %ERRORLEVEL% NEQ 0 (
    ECHO Error: xxd was not found, please install and make sure it is in PATH
    exit /B 1
)

WHERE meson
IF %ERRORLEVEL% NEQ 0 (
    ECHO Error: meson was not found, please install and make sure it is in PATH
    exit /B 1
)

:: Change prompt
set PROMPT=[ENV] $P$G