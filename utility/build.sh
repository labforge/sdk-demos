#!/bin/bash
##############################################################################
#  Copyright 2023 Labforge Inc.                                              #
#                                                                            #
# Licensed under the Apache License, Version 2.0 (the "License");            #
# you may not use this project except in compliance with the License.        #
# You may obtain a copy of the License at                                    #
#                                                                            #
#     http://www.apache.org/licenses/LICENSE-2.0                             #
#                                                                            #
# Unless required by applicable law or agreed to in writing, software        #
# distributed under the License is distributed on an "AS IS" BASIS,          #
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.   #
# See the License for the specific language governing permissions and        #
# limitations under the License.                                             #
##############################################################################
BASEDIR=$(dirname $0)
pushd "${BASEDIR}"
pyside6-uic -g python -o widgets.py *.ui
pyside6-rcc -g python -o resources_rc.py resources.qrc
popd