:: FIXME: Cover this somehow in NSIS
::=============================================================================
:: Script: bundle.bat
:: Author: Thomas Reidemeister <thomas@labforge.ca>
::   This script bootstraps the needed OSS dependencies for Windows builds.
::
::   Requirements:
::     Powershell
::     Python > 3.7 (and in path)
::     CMAKE (and in path)
::     GIT (and in path)
::     MS Build tools configured for Visual C++ (Desktop), https://aka.ms/vs/17/release/vs_buildtools.exe
::
::   Installations of the Script
::     Visual C++ 2022 Build Tools -> %ProgramFiles(x86)%\Microsoft Visual Studio\
::     QT5 5.1.12 LGPL components  -> %ProgramFiles\qt5
::     OpenCV 4.3.0                -> %ProgramFiles\cv4
::     XXD                         -> C:\Windows\System32\
::
:: Copyright (C) 2013-2021 Labforge Inc.
::
:: THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
:: AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
:: IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
:: ARE DISCLAIMED. IN NO EVENT SHALL COPYRIGHT HOLDERS AND CONTRIBUTORS BE
:: LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
:: CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
:: SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
:: INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
:: CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
:: ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
:: POSSIBILITY OF SUCH DAMAGE.
::
::=============================================================================
@ECHO OFF

:: Setup and check dependencies first
CALL %~dp0\env.bat

:: Build release with meson
MESON --buildtype=release --prefix=%~dp0\install build_dist
IF %ERRORLEVEL% NEQ 0 goto :exit_with_error
NINJA -C build_dist all
IF %ERRORLEVEL% NEQ 0 goto :exit_with_error
NINJA -C build_dist install
IF %ERRORLEVEL% NEQ 0 goto :exit_with_error

:: COPY QT5 dlls
FOR %%M IN (
    Core
    Gui
    Widgets
    OpenGL
) DO (
    COPY /Y %QTHOME%\bin\QT5%%M.dll %~dp0\install\bin
    IF %ERRORLEVEL% NEQ 0 goto :exit_with_error
)
COPY /Y %QTHOME%\bin\libGLESv2.dll %~dp0\install\bin
IF %ERRORLEVEL% NEQ 0 goto :exit_with_error
:: COPY required Windows Plugin for distribution
MD %~dp0\install\bin\platforms
COPY /Y %QTHOME%\plugins\platforms\qwindows.dll %~dp0\install\bin\platforms
IF %ERRORLEVEL% NEQ 0 goto :exit_with_error
MD %~dp0\install\bin\styles
COPY /Y %QTHOME%\plugins\styles\qwindowsvistastyle.dll %~dp0\install\bin\styles
IF %ERRORLEVEL% NEQ 0 goto :exit_with_error
MD %~dp0\install\bin\imageformats
COPY /Y %QTHOME%\plugins\imageformats\qico.dll %~dp0\install\bin\imageformats
COPY /Y %QTHOME%\plugins\imageformats\qjpeg.dll %~dp0\install\bin\imageformats

:: COPY OPENCV dlls
FOR %%M IN (
    opencv_core
    opencv_highgui
    opencv_imgcodecs
    opencv_imgproc
    opencv_calib3d
	opencv_features2d
	opencv_flann
) DO (
    COPY /Y %OPENCVHOME%\bin\%%M*.dll %~dp0\install\bin
    IF %ERRORLEVEL% NEQ 0 goto :exit_with_error
    echo "Past %OPENCVHOME%\bin\%%M*.dll"
)

:: COPY Pleora SDK Files
copy /Y "C:\Program Files\Common Files\Pleora\eBUS SDK\GenICam\bin\Win64_x64\*.dll" ^
	%~dp0\install\bin
ECHO "Genicam copied"
copy /Y "C:\Program Files\Common Files\Pleora\eBUS SDK\*64.dll" %~dp0\install\bin
ECHO "PV copied"
copy /Y "C:\Program Files\Common Files\Pleora\eBUS SDK\*64_VC16.dll" %~dp0\install\bin
ECHO "GUI copied"

GOTO :normal_end

:exit_with_error
    ECHO "Error encountered ... exiting"
    exit /B 1

:normal_end
