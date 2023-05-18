@ECHO OFF
:: Main assembly script for Python distribution
::    Copyright 2023 Labforge Inc.
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

:: build driver installer
CD ..\driver\
CALL build.bat
:: Build utility Qt python scripts
CD ..\utility\
CALL build.bat
CD ..\distribution
:: build python modules, including the driver installer
python setup.py build
:: build python modules, including the driver installer
python setup.py installer