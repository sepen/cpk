# configure.ac

AC_INIT([cpk], [0.1], [sepen@crux.nu])
AM_INIT_AUTOMAKE([foreign -Wall -Werror])
AC_PROG_CXX
AC_PROG_INSTALL

# Check for required headers
AC_CHECK_HEADERS([algorithm archive.h archive_entry.h cstdlib curl/curl.h filesystem fstream iostream sstream string sys/stat.h vector stdexcept iomanip])

# Check for libarchive
PKG_CHECK_MODULES([LIBARCHIVE], [libarchive], [], [
    AC_MSG_ERROR([libarchive not found. Install it with 'sudo apt install libarchive-dev' or 'pkg install libarchive'])
])

# Check for libcurl
PKG_CHECK_MODULES([LIBCURL], [libcurl], [], [
    AC_MSG_ERROR([libcurl not found. Install it with 'sudo apt install libcurl4-openssl-dev' or 'pkg install curl'])
])

# ------------------------------------------------------------------
# Detect GCC version and set the appropriate C++ standard
# ------------------------------------------------------------------
AC_MSG_CHECKING([for GCC version])
GCC_VERSION=`$CXX -dumpversion 2>/dev/null`
AC_MSG_RESULT([$GCC_VERSION])

# Remove any pre-set C++ standard flags
CXXFLAGS=`echo "$CXXFLAGS" | sed 's/-std=c++[0-9a-z]*//g'`

if test "x$GCC_VERSION" != "x"; then
    GCC_MAJOR=`echo $GCC_VERSION | cut -d. -f1`

    if test "$GCC_MAJOR" -lt 9; then
        AC_MSG_NOTICE([Detected GCC < 9 → using -std=c++2a and linking with -lstdc++fs])
        CXXFLAGS="$CXXFLAGS -std=c++2a"
        LIBS="$LIBS -lstdc++fs"
    else
        AC_MSG_NOTICE([Detected GCC >= 9 → using -std=c++20])
        CXXFLAGS="$CXXFLAGS -std=c++20"
    fi
else
    AC_MSG_WARN([Unable to detect GCC version → defaulting to -std=c++20])
    CXXFLAGS="$CXXFLAGS -std=c++20"
fi

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
