from setuptools import setup, find_packages, Extension
from setuptools.command.build_ext import build_ext
import os

# Definitions used to generate distribution
NAME = "eBus Driver Update Utility for Windows"
SHORTNAME = "ebus_driver_update"
DESCRIPTION = "A tool to update the eBus driver for Labforge Bottlenose cameras"
VERSION = "0.1.1"
URL = "https://labforge.ca"
AUTHOR = "Thomas Reidemeister"
EMAIL = "thomas@labforge.ca"
COMPANY = "Labforge Inc"


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
              sources=['driver.cc'],
    )
]

setup(
    name=NAME,
    version=VERSION,
    url=URL,
    author=AUTHOR,
    author_email=EMAIL,
    python_requires='>=3.7',
    packages=find_packages("."),
    package_dir={""},
    entry_points={
        "gui_scripts": [
            "updater = main:main",
        ],
    },
    cmdclass={
        'build_ext': BuildExt,
    },
    ext_modules=ext_modules,
    setup_requires="setuptools",
    install_requires=[
        # eBusPython solved externally
    ],
    extras_require={
        "installer": [],
    }
)
