::=============================================================================
:: Script: env.bat
:: Author: Thomas Reidemeister <thomas@labforge.ca>
::   This script bootstraps the paths and needed tools for Windows builds/development.
::
::   Requirements:
::     Powershell
::     Python > 3.7 (and in path)
::     WixToolset
::     MS Build tools configured for Visual C++ (Desktop), https://aka.ms/vs/17/release/vs_buildtools.exe
::
::   Installations of the Script
::     Visual C++ 2022 Build Tools -> %ProgramFiles(x86)%\Microsoft Visual Studio\
::     QT5 5.1.12 LGPL components  -> %ProgramFiles\qt5
::     QWT 6.2.0                   -> %ProgramFiles\qwt-6.2.0
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
SET QTHOME=C:\qt5
IF NOT EXIST %QTHOME% (
    ECHO QT5 not properly installed ... exiting
    EXIT /B 1
) ELSE (
    SET "PATH=%QTHOME%\bin;%QTHOME%\lib;%PATH%"
    ECHO QT5 configured
)

:: QWT6
SET QWTHOME=C:\qwt-6.2.0\
IF NOT EXIST %QWTHOME% (
    ECHO QWT not properly installed ... exiting
    EXIT /B 1
) ELSE (
    SET "PATH=%QWTHOME%\include;%QWTHOME%\bin;%QWTHOME%\lib;%PATH%"
)

:: OpenCV4
SET OPENCVHOME="C:\cv4"
IF NOT EXIST %OPENCVHOME% (
    ECHO OpenCV not properly installed ... exiting
    EXIT /B 1
) ELSE (
    SET "PATH=%OPENCVHOME%\bin;%OPENCVHOME%\lib;%PATH%"
)

WHERE python
IF %ERRORLEVEL% NEQ 0 (
    ECHO Error: python was not found, please install and make sure it is in PATH
    exit /B 1
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