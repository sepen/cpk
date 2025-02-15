# configure.ac
AC_INIT([cpk], [0.1], [sepen@crux-arm.nu])
AM_INIT_AUTOMAKE([foreign -Wall -Werror])
AC_PROG_CXX
AC_PROG_INSTALL

AC_CHECK_HEADERS([algorithm archive.h archive_entry.h cstdlib curl/curl.h filesystem fstream iostream sstream string sys/stat.h vector stdexcept iomanip])

PKG_CHECK_MODULES([LIBARCHIVE], [libarchive], [], [
    AC_MSG_ERROR([libarchive not found. Install it with 'sudo apt install libarchive-dev' or 'brew install libarchive'])
])

PKG_CHECK_MODULES([LIBCURL], [libcurl], [], [
    AC_MSG_ERROR([libcurl not found. Install it with 'sudo apt install libcurl4-openssl-dev' or 'brew install curl'])
])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT

