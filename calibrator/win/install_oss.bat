::=============================================================================
:: Script: bootstrap_oss.bat
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
::     QT5 5.1.12 LGPL components  -> C:\qt5
::     QWT 6.2.0                   -> C:\qwt-6.2.0
::     OpenCV 4.3.0                -> C:\cv4
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

:: See that needed tools are already installed
WHERE git
IF %ERRORLEVEL% NEQ 0 (
    ECHO Error: git was not found, please install and make sure it is in PATH
    exit /B 1
)
WHERE cmake
IF %ERRORLEVEL% NEQ 0 (
    ECHO Error: cmake was not found, please install and make sure it is in PATH
    exit /B 1
)
WHERE python
IF %ERRORLEVEL% NEQ 0 (
    ECHO Error: python was not found, please install and make sure it is in PATH
    exit /B 1
)

:: Install VS Build tools
IF NOT EXIST "%ProgramFiles(x86)%\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" (
    ECHO Please install VC Build tools for Visual C++ https://aka.ms/vs/17/release/vs_buildtools.exe
    exit /B 1
) 
:: Configure environment
CALL "%ProgramFiles(x86)%\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" x64

:: Install QT5
SET "QT5HOME=C:\qt5"
IF NOT EXIST %QT5HOME% (
    :: Install StawberryPerl
    IF NOT EXIST "perl\" (
        CURL -SL --output strawberry-perl-5.32.1.1-64bit.zip https://strawberryperl.com/download/5.32.1.1/strawberry-perl-5.32.1.1-64bit.zip
        POWERSHELL Expand-Archive strawberry-perl-5.32.1.1-64bit.zip -DestinationPath perl && del /q strawberry-perl-5.32.1.1-64bit.zip
    ) ELSE (
        ECHO Perl already installed locally
    )
    SET "PATH=%~dp0perl\perl\bin;%PATH%"
    IF NOT EXIST "qt5src\" (
        GIT clone https://code.qt.io/qt/qt5.git --single-branch --branch 5.12 qt5src
        CD qt5src
        PERL init-repository -f --module-subset="essential"
        CD ..
    ) ELSE (
        ECHO QT5 Source already configured ... skipping
    )
    IF NOT EXIST "qt5build\" (
        MD qt5build
        CD qt5build
        ..\qt5src\configure -prefix "C:\qt5" -confirm-license -opensource -nomake examples -nomake tests
        nmake
        nmake install
        CD ..
    )
) ELSE (
    SET "PATH=%QT5HOME%\bin;%QT5HOME%\lib;%PATH%"
)

:: Install QWT
SET "QWTHOME=C:\qwt-6.2.0\"
IF NOT EXIST %QWTHOME% (
    IF NOT EXIST "qwt-6.2.0\" (
        CURL -SL --output qwt-6.2.0.zip http://downloads.sourceforge.net/project/qwt/qwt/6.2.0/qwt-6.2.0.zip
        POWERSHELL Expand-Archive qwt-6.2.0.zip -DestinationPath . && del /q qwt-6.2.0.zip
        :: Reduce scope of QWT install
        COPY /Y qwtconfig.pri qwt-6.2.0
        CD qwt-6.2.0
        QMAKE qwt.pro
        NMAKE
        NMAKE install
        CD ..
    ) ELSE (
        ECHO QWT not properly installed ... exiting
        EXIT /B 1
    )    
) ELSE (
    SET "PATH=%QWTHOME%bin;%QWTHOME%lib;%PATH%"
)

:: Install OpenCV4.3.0
SET "OPENCVHOME=C:\cv4\"
IF NOT EXIST %OPENCVHOME% (
    IF NOT EXIST opencv\ (
        GIT clone https://github.com/opencv/opencv --single-branch --branch 4.3.0
        MD cv4build
        CD cv4build
        CMAKE ..\opencv -D CMAKE_BUILD_TYPE=RELEASE ^
            -D BUILD_DOCS=OFF ^
            -D BUILD_PERF_TESTS=OFF ^
            -D BUILD_TESTS=OFF ^
            -DOPENCV_ALLOCATOR_STATS_COUNTER_TYPE=int64_t ^
	    -G "NMake Makefiles" ^
            -D CMAKE_INSTALL_PREFIX=C:\cv4\

        NMAKE
        NMAKE install
    ) ELSE (
        ECHO OpenCV not properly installed ... exiting
	EXIT /B 1
    )
) ELSE (
    SET "PATH=%OPENCVHOME%bin;%OPENCVHOME%lib;%PATH%"
)