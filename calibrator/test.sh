#!/bin/sh
##
# Test runner for CI coverage in a SBT scenario (not on-target)
# Copyright (C) 2013-2021 Labforge Inc.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL COPYRIGHT HOLDERS AND CONTRIBUTORS BE
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.

REQUIRED_STATEMENT_COVERAGE_PERCENT=0

# Make sure pleora Env is setup for test cases
export DIRNAME=/opt/pleora/ebus_sdk/Ubuntu-18.04-x86_64/
export PUREGEV_ROOT=$DIRNAME
export GENICAM_ROOT=$DIRNAME/lib/genicam
export GENICAM_ROOT_V3_1=$GENICAM_ROOT
export GENICAM_LOG_CONFIG=$DIRNAME/lib/genicam/log/config/DefaultLogging.properties
export GENICAM_LOG_CONFIG_V3_1=$GENICAM_LOG_CONFIG
export GENICAM_CACHE_V3_1=$HOME/.config/Pleora/genicam_cache_v3_1
export GENICAM_CACHE=$GENICAM_CACHE_V3_

# Make sure venv is set up
[ ! -d "./venv" ] && python3.6 -m venv venv

. ./venv/bin/activate
pip install --upgrade -r requirements.txt

# Remove build_test if exists
meson build_tests --buildtype=release -Db_coverage=true
cd build_tests
meson compile
set -e
meson test --verbose
ninja coverage

# Expand coverage
if [ "$(cat meson-logs/coverage.txt | grep TOTAL | awk '{ print substr($4, 1, length($4)-1) }')" -lt ${REQUIRED_STATEMENT_COVERAGE_PERCENT} ];
then
  echo "Insufficient test coverage, see below"
  cat meson-logs/coverage.txt
  exit 1
else
  echo "Test coverage sufficient"
  cat meson-logs/coverage.txt
fi

