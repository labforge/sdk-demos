@ECHO OFF
REM ##############################################################################
REM #  Copyright 2023 Labforge Inc.                                              #
REM #                                                                            #
REM # Licensed under the Apache License, Version 2.0 (the "License");            #
REM # you may not use this project except in compliance with the License.        #
REM # You may obtain a copy of the License at                                    #
REM #                                                                            #
REM #     http://www.apache.org/licenses/LICENSE-2.0                             #
REM #                                                                            #
REM # Unless required by applicable law or agreed to in writing, software        #
REM # distributed under the License is distributed on an "AS IS" BASIS,          #
REM # WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.   #
REM # See the License for the specific language governing permissions and        #
REM # limitations under the License.                                             #
REM ##############################################################################
REM @author Thomas Reidemeister
REM
REM This script sets up the requirements for this repository

REM Check that venv is bootstrapped
IF NOT EXIST venv\ (
  ECHO "Setting up python virtual environment"
  python -m venv venv
)
CALL venv\Scripts\activate

REM setup dependencies
pip install -r distribution\requirements.txt
pip install -r stream\requirements.txt
pip install -r utility\requirements.txt