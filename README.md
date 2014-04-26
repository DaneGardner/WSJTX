# WSJT-X by W7DLG

This is a fork of the original project, as they are doing a lot of work to it right now, and I really just wanted to hotfix a couple of longstanding bugs immediately.  It is not my intention to support this long term (or at all, really).


## Changes
* Displays exact messages being transmitted, instead of surprising users with changes at transmit time
  * differences are highlighted using red background color, and a tooltip displaying the original desired message
* Fixed the highlighting of "free text" messages
* Support proper use of prefix and suffix in standard message generation
  * as per Joe Taylor (K1JT) in his paper on JT65 (I don't know why the project deviated from his specs)
* Support proper identification of user's callsign and base callsign
  * if you had a suffix in your callsign, and someone properly responded using your base callsign, the software ignored it entirely
* Being a Qt developer, I'm using proper Qt coding practices in areas that I rewrite
  * there's still a lot of 'C' style programming in here; that's bad.
* Enhanced (slightly) CMake building
  * uses FFTW3_ROOT_DIR and HAMLIB_ROOT_DIR for hints when finding the includes and library files


## Building
Follow the [WSJT Developer's Guide](http://www.physics.princeton.edu/pulsar/K1JT/wsjtx-doc/wsjt-dev-guide.html) for the prerequisites.  Then, using MinGW's Bash in Windows:

```bash
git clone -b 1.3 --single-branch https://github.com/DaneGardner/WSJTX.git wsjtx-dg
 
export PATH="/d/wsjt-dev/cmake/bin:/c/Qt/5.2.1/mingw48_32/bin:/c/Qt/Tools/mingw48_32/bin:$HOME/bin:/usr/local/bin:/mingw/bin:/bin"

mkdir wsjtx-build
pushd wsjtx-build
  rm -rf *

  # Ensure that sh.exe is not in the executable path when using the "MinGW Makefile" generator in CMake
  PATH="/d/wsjt-dev/cmake/bin:/d/wsjt-dev/hamlib/bin:/c/Qt/5.2.1/mingw48_32/bin:/c/Qt/Tools/mingw48_32/bin:$HOME/bin" \
  CC="/c/Qt/Tools/mingw48_32/bin/gcc" \
  CXX="/c/Qt/Tools/mingw48_32/bin/g++" \
    cmake -G "MinGW Makefiles" \
      -DCMAKE_INSTALL_PREFIX:PATH="../wsjtx-install" \
      -DCMAKE_LIBRARY_PATH="/c/Windows/System32" \
      -DFFTW3_ROOT_DIR="../fftw" \
      -DHAMLIB_ROOT_DIR="../hamlib" \
      "../wsjtx-dg"

  mingw32-make all

  mingw32-make install
popd
```

You'll also want to find and copy over the following files to the bin directory from your Qt/MinGW installation:
* icudt51.dll
* icuin51.dll
* icuuc51.dll
* libgcc_s_dw2-1.dll
* libgfortran-3.dll
* libquadmath-0.dll
* libstdc++-6.dll
* libwinpthread-1.dll
* Qt5Core.dll
* Qt5Gui.dll
* Qt5Multimedia.dll
* Qt5Network.dll
* Qt5Widgets.dll
* platforms/qwindows.dll
