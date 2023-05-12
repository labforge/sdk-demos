@ECHO OFF
REM Main assembly script for Python distribution
REM    Copyright 2023 Labforge Inc.
REM
REM    Licensed under the Apache License, Version 2.0 (the "License");
REM    you may not use this file except in compliance with the License.
REM    You may obtain a copy of the License at
REM
REM        http://www.apache.org/licenses/LICENSE-2.0
REM
REM    Unless required by applicable law or agreed to in writing, software
REM    distributed under the License is distributed on an "AS IS" BASIS,
REM    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
REM    See the License for the specific language governing permissions and
REM    limitations under the License.

REM build driver installer
CD ..\driver\
CALL build.bat
REM Build utility Qt python scripts
CD ..\utility\
CALL build.bat
CD ..\distribution
REM build python modules, including the driver installer
python setup.py build
REM build python modules, including the driver installer
python setup.py installer