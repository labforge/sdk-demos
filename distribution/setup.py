# coding: utf-8
"""
******************************************************************************
*  Copyright 2023 Labforge Inc.                                              *
*                                                                            *
* Licensed under the Apache License, Version 2.0 (the "License");            *
* you may not use this project except in compliance with the License.        *
* You may obtain a copy of the License at                                    *
*                                                                            *
*     http://www.apache.org/licenses/LICENSE-2.0                             *
*                                                                            *
* Unless required by applicable law or agreed to in writing, software        *
* distributed under the License is distributed on an "AS IS" BASIS,          *
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.   *
* See the License for the specific language governing permissions and        *
* limitations under the License.                                             *
******************************************************************************
"""
__author__ = "Thomas Reidemeister <thomas@labforge.ca>"
__copyright__ = "Copyright 2023, Labforge Inc."

from setuptools import setup, find_packages, Extension
from setuptools.command.build_ext import build_ext
from distutils.cmd import Command
import pkg_resources
import subprocess
import os

# Definitions used to generate distribution
NAME = "Bottlenose Utilities"
SHORTNAME = "updater"
DESCRIPTION = "A tool to manage Labforge Bottlenose Cameras"
VERSION = "0.1.1"
URL = "https://labforge.ca"
AUTHOR = "Thomas Reidemeister"
EMAIL = "thomas@labforge.ca"
COMPANY = "Labforge Inc" # Do not add "." this is used for directory names


class BuildApp(Command):
    description = 'Build the application using PyInstaller'
    user_options = [
        ('src=', None, 'Path to the Python sources'),
        ('spec=', None, 'Path to spec file for PyInstaller'),
    ]

    def initialize_options(self):
        script_path = os.path.abspath(__file__)
        script_dir = os.path.dirname(script_path)
        self.src = script_dir
        self.spec = 'install.spec'

    def render_version(self):
        from jinja2 import Environment, FileSystemLoader
        env = Environment(loader=FileSystemLoader(self.src))
        template = env.get_template('version.j2')
        output = template.render(company=COMPANY,
                                 description=DESCRIPTION,
                                 version=VERSION,
                                 name=NAME,
                                 short_name=SHORTNAME)
        with open(os.path.join(self.src, 'version.txt'), 'w') as f:
            f.write(output)


    def finalize_options(self):
        pass

    def run(self):
        # Generate version file for the executable
        self.render_version()
        # Generate the executable
        subprocess.check_call(['pyinstaller', '--clean', '-y', self.spec])


class BuildInstaller(Command):
    description = 'Build the Installer using NSIS'
    user_options = [
        ('src=', None, 'Path to the Python sources'),
        ('out', None, 'Output file for the executable'),
        ('script', None, 'Path to the NSIS sources'),
    ]

    def initialize_options(self):
        script_path = os.path.abspath(__file__)
        script_dir = os.path.dirname(script_path)
        self.src = script_dir
        self.script = 'install.nsi'
        self.out = os.path.join(script_dir, 'build', SHORTNAME+'.exe')


    def finalize_options(self):
        pass

    def run(self):
        # Generate the installer
        subprocess.check_call(['makensis',
                               '-DSRC=%s' % self.src,
                               '-DOUT=%s' % self.out,
                               '-DCOMPANY='+COMPANY,
                               '-DNAME=' + NAME,
                               '-DSHORTNAME=' + SHORTNAME,

                               self.script])


# Compile external modules as executables
class BuildExt(build_ext):
    def build_extensions(self):
        for ext in self.extensions:
            # Interceipt driver app as freestanding compilation against installed Pleora SDK
            if ext.name == 'driver':
                self.compiler.add_include_dir('C:\\Program Files\\Pleora Technologies Inc\\eBUS SDK\\Includes')
                self.compiler.add_library_dir('C:\\Program Files\\Pleora Technologies Inc\\eBUS SDK\\Libraries')
                self.compiler.compile([ext.sources[0]])


                obj_filename = self.compiler.object_filenames(ext.sources)[0]
                ext_path = os.path.join('build', 'driver')
                self.compiler.link_executable([obj_filename], ext_path)
            else:
                self.build_extension(ext)


# DriverAPI sample
ext_modules = [
    Extension('driver',
              sources=['../driver/driver.cc'],
    )
]

setup(
    name=NAME,
    version=VERSION,
    url=URL,
    author=AUTHOR,
    author_email=EMAIL,
    python_requires='>=3.7',
    packages=find_packages("../utility"),
    package_dir={"": "../utility"},
    entry_points={
        "gui_scripts": [
            "updater = main:main",
        ],
    },
    cmdclass={
        'build_app': BuildApp,
        'build_installer': BuildInstaller,
        'build_ext': BuildExt,
    },
    ext_modules=ext_modules,
    setup_requires="setuptools",
    install_requires=[
        "PySide6>=6.4.3",
        "opencv-python-headless>=4.7.0.72",
        "pyyaml>=5.4.1"
        # eBusPython solved externally
    ],
    extras_require={
        "installer": ["pyinstaller>=5.9",
                      "pyinstaller",
                      "Jinja2>=3.1.2",
                      "Jinja2"],
    }
)
