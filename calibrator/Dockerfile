# syntax=docker/dockerfile:1
###############################################################################
# Script: Docker file to setup build environment for Ubuntu 18.04 with Pleora SDK
#
# Requirements:
# - Place the eBUS_SDK_Ubuntu-18.04-x86_64-6.2.5-5601.deb in the same directory
#   as this script when creating the docker image
#
# Copyright (C) 2013-2022 Labforge Inc.
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
#
###############################################################################
# docker push labforge/bottlenose_suite_build:tagname
FROM ubuntu:18.04
LABEL maintainer="thomas@labforge.ca"

# Install Dependencies
COPY eBUS_SDK_Ubuntu-18.04-x86_64-6.2.5-5601.deb /
ARG DEBIAN_FRONTEND=noninteractive
ENV TZ=Etc/UTC
RUN echo "deb-src http://archive.ubuntu.com/ubuntu bionic main" >> /etc/apt/sources.list && \
    echo "deb-src http://archive.ubuntu.com/ubuntu bionic-updates main" >> /etc/apt/sources.list && \
    apt-get update && \
    apt-get install --no-install-recommends -y software-properties-common && \
    add-apt-repository ppa:ubuntu-toolchain-r/test && \
    apt-get install --no-install-recommends -y autoconf automake libtool curl make cmake g++ unzip \
                      build-essential pkg-config python3 python3-pip \
                      python3-venv wget git libqwt-qt5-dev libqwt-headers libqwt-qt5-6 \
                      qt5-qmake libqt5gui5 libqt5widgets5 libqt5test5 libqt5core5a qtbase5-dev qtbase5-dev-tools \
                      xvfb x11-utils libxkbcommon-x11-0 libxcb-icccm4 libxcb-image0 libxcb-keysyms1 libxcb-randr0 \
                      libxcb-render-util0 libxcb-xinerama0 libxcb-xfixes0 xdotool udhcpc net-tools \
                      libiberty-dev autoconf xxd ninja-build \
                      gcc-10 g++-10 \
                      libavcodec57 libavutil55 openssh-client && \
    update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-10 10 && \
    update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-10 10 && \
    update-alternatives --install /usr/bin/gcov gcov-10 /usr/bin/gcov-10 10 && \
    dpkg -i /eBUS_SDK_Ubuntu-18.04-x86_64-6.2.5-5601.deb && \
    apt-get --no-install-recommends -f install && \
    pip3 install meson

# Install OpenCV 4.3.0 \
RUN git clone https://github.com/opencv/opencv/ --single-branch --branch 4.3.0 && \
    cd opencv && \
    mkdir build && \
    cd build && \
    cmake -D CMAKE_BUILD_TYPE=RELEASE -D OPENCV_GENERATE_PKGCONFIG=ON -D BUILD_DOCS=OFF -D BUILD_PERF_TESTS=OFF -D BUILD_TESTS=OFF .. && \
    make install && \
    cd .. && \
    cd ..

