from distutils.core import setup, Extension
import sys

if sys.version_info[0] < 3:
    print("PyPoly requires Python >= 3")
    sys.exit()

pypoly_version = '0.1.0'

with open('README.md') as f:
    long_description = f.read()

pypoly_module = Extension(
                    "pypoly",
                    ["pypoly/polynomials.c", "pypoly/pypoly.c"],
                    define_macros=[
                        ('PYPOLY_VERSION', pypoly_version)])

setup(name="PyPolynomial",
      description="Python polynomial C extension.",
      long_description=long_description,
      version=pypoly_version,
      ext_modules=[pypoly_module,],
      author="Thomas Chaumeny",
      author_email="t.chaumeny@gmail.com",
      url="https://github.com/tchaumeny/PyPoly")
