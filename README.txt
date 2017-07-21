Handel-SiToro is built with the python-based waf. You will need to install
Python. waf is included in the source in platform/waf/waf.

# Compilers

The waf script uses compiler_c to find a compiler. We support msvc and gcc.

## msvc

We require Visual Studio 2013 because some source files use C99-style
initializers.

If configuring for msvc x64, be sure to specify "amd64" as an argument to
vcvarsall.bat. This is easy to miss because waf acts like it figures out all
the paths during configure, but if you miss it you may get a link error about
missing symbol _DllMainCRTStartup.

"c:/Program Files (x86)/Microsoft Visual Studio 12.0/VC/vcvarsall.bat" amd64

## gcc

On Windows, download msys from https://msys2.github.io and follow the steps.
Add gcc with `pacman -S gcc`. Configure with `--check-c-compiler=gcc` to step
around the default msvc.

# Configure

If linking to sitoro for FalconX (FalconXN does not need this), configure with
`--sitoro=PATH_TO_MANU_DIR`, e.g. c:\siToroPackage_2.5.1\manu\sitoro-2.5.1.

Force not linking to sitoro with `--sitoro=disable`.

# Running C tests

1. PATH=%PATH%;...siToroPackage_2.5.1\manu\sitoro-2.5.1\bin
2. build-win32\debug\hd-*.exe
